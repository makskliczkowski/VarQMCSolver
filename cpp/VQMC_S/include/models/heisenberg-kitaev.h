#pragma once

#include "heisenberg.h"


// --------------------------------------------------------------------------- HEISENBERG INTERACTING WITH KITAEV SPINS ---------------------------------------------------------------------------
#ifndef HEISENBERG_KITAEV
#define HEISENBERG_KITAEV
template<typename _type>
class Heisenberg_kitaev : public Heisenberg<_type> {
private:
	double Kx = 1.0;									// kitaev model exchange 
	double Ky = 1.0;									// kitaev model exchange 
	double Kz = 1.0;									// kitaev model exchange 
	vec dKx;											// kitaev model exchange vector
	vec dKy;											// kitaev model exchange vector
	vec dKz;											// kitaev model exchange vector
	double K0;											// disorder with Kitaev exchange
	vec tmp_vec;
	vec tmp_vec2;
public:
	~Heisenberg_kitaev() = default;
	Heisenberg_kitaev(double J, double J0, double g, double g0, double h, double w, double delta, std::tuple<double, double, double> K, double K0, std::shared_ptr<Lattice> lat)
		: Heisenberg<_type>(J, J0, g, g0, h, w, delta, lat)
	{
		this->Kx = std::get<0>(K);
		this->Ky = std::get<1>(K);
		this->Kz = std::get<2>(K);
		this->K0 = K0;
		// creates random disorder vector
		this->dKx = create_random_vec(this->Ns, this->ran, this->K0);						
		this->dKy = create_random_vec(this->Ns, this->ran, this->K0);
		this->dKz = create_random_vec(this->Ns, this->ran, this->K0);
		this->loc_states_num = 1 + this->Ns * (1 + lat->get_nn_number(0));													// number of states after local energy work
		this->locEnergies = v_1d<std::pair<u64, _type>>(this->loc_states_num, std::make_pair(LONG_MAX, 0));					// set local energies vector
		// change info
		this->info = this->inf();
	};
	// ----------------------------------- SETTERS ---------------------------------

	// ----------------------------------- GETTERS ---------------------------------
	void locEnergy(u64 _id) override;
	void locEnergy(const vec& v) override;
	void hamiltonian() override;

	string inf(const v_1d<string>& skip = {}, string sep = "_") const override
	{
		string name = sep + \
			"hei_kitv,Ns=" + STR(this->Ns) + \
			",J=" + STRP(this->J, 2) + \
			",J0=" + STRP(this->J0, 2) + \
			",d=" + STRP(this->delta, 2) + \
			",g=" + STRP(this->g, 2) + \
			",g0=" + STRP(this->g0, 2) + \
			",h=" + STRP(this->h, 2) + \
			",w=" + STRP(this->w, 2) + \
			",K=(" + STRP(this->Kx, 2) + "," + STRP(this->Ky,2) + "," + STRP(this->Ky, 2) + ")" \
			",K0=" + STRP(this->K0, 2);
		return SpinHamiltonian::inf(name, skip, sep);
	}
};

// ----------------------------------------------------------------------------- LOCAL ENERGY -------------------------------------------------------------------------------------

/*
* @brief Calculate the local energy end return the corresponding vectors with the value
* @param _id base state index
*/
template <typename _type>
inline void Heisenberg_kitaev<_type>::locEnergy(u64 _id) {

	// sumup the value of non-changed state
	double localVal = 0;

	// check all the neighbors
#ifndef DEBUG
#pragma omp parallel for reduction(+ : localVal)
#endif // !DEBUG
	for (auto i = 0; i < this->Ns; i++) {

		auto nn_number = this->lattice->get_nn_number(0);

		// true - spin up, false - spin down
		double si = checkBit(_id, this->Ns - i - 1) ? 1.0 : -1.0;

		// perpendicular field (SZ) - HEISENBERG
		localVal += (this->h + this->dh(i)) * si;

		// transverse field (SX) - HEISENBERG
		const u64 new_idx = flip(_id, this->Ns - 1 - i);
		this->locEnergies[i] = std::make_pair(new_idx, this->g + this->dg(i));

		// check the correlations
		for (auto n_num = 0; n_num < nn_number; n_num++) {
			if (auto nn = this->lattice->get_nn(i, n_num); nn >= 0) {//&& nn >= i

				// check Sz 
				double sj = checkBit(_id, this->Ns - 1 - nn) ? 1.0 : -1.0;
				
				// --------------------- HEISENBERG 
				
				// diagonal elements setting  interaction field
				double interaction = this->J + this->dJ(i);
				double sisj = si * sj;
				localVal += interaction * this->delta * sisj;
				
				const u64 flip_idx_nn = flip(new_idx, this->Ns - 1 - nn);
				// set element of the local energies
				const int elem = (n_num + 1) * this->Ns + i;
				double flip_val = 0.0;

				// S+S- + S-S+
				if (sisj < 0)
					//this->locEnergies[(n_num+1)*this->Ns + i] = std::make_pair(flip_idx_nn, 0.5 * interaction);
					flip_val += 0.5 * interaction;

				// --------------------- KITAEV
				if (n_num == 0)
					localVal += (this->Kz + this->dKz(i)) * sisj;
				else if (n_num == 1)
					//this->locEnergies[2 * this->Ns + i] = std::make_pair(flip_idx_nn, -(this->Ky + this->dKy(i)) * sisj);
					flip_val -= (this->Ky + this->dKy(i)) * sisj;

				else if (n_num == 2)
					//this->locEnergies[3 * this->Ns + i] = std::make_pair(flip_idx_nn, this->Kx + this->dKx(i));
					flip_val += this->Kx + this->dKx(i);
				
				this->locEnergies[elem] = std::make_pair(flip_idx_nn, flip_val);

			}
		}
	}
	// append unchanged at the very end
	this->locEnergies[this->loc_states_num-1] = std::make_pair(_id, static_cast<_type>(localVal));
}

/*
* @brief Calculate the local energy end return the corresponding vectors with the value
* @param _id base state index
*/
template <typename _type>
void Heisenberg_kitaev<_type>::locEnergy(const vec& v) {


	// sumup the value of non-changed state
	double localVal = 0;
	for (auto i = 0; i < this->Ns; i++) {
		// check all the neighbors

		auto nn_number = this->lattice->get_nn_number(i);

		// true - spin up, false - spin down
		double si = checkBitV(v, i) > 0 ? 1.0 : -1.0;

		// perpendicular field (SZ) - HEISENBERG
		localVal += (this->h + this->dh(i)) * si;

		// transverse field (SX) - HEISENBERG
		this->tmp_vec = v;
		flipV(tmp_vec, i);
		const u64 new_idx = baseToInt(tmp_vec);
		this->locEnergies[i] = std::pair{ new_idx, this->g + this->dg(i) };

		// check the correlations
		for (auto n_num = 0; n_num < nn_number; n_num++) {
			this->tmp_vec2 = this->tmp_vec;
			if (auto nn = this->lattice->get_nn(i, n_num); nn >= 0) {//&& nn >= i
				// stout << VEQ(i) << ", nei=" << VEQ(nn) << EL;
				// check Sz 
				double sj = checkBitV(v, nn) > 0 ? 1.0 : -1.0;

				// --------------------- HEISENBERG 

				// diagonal elements setting  interaction field
				auto interaction = this->J + this->dJ(i);
				auto sisj = si * sj;
				localVal += interaction * this->delta * sisj;

				flipV(tmp_vec2, nn);
				auto flip_idx_nn = baseToInt(tmp_vec2);

				// S+S- + S-S+
				if (sisj < 0)
					this->locEnergies[this->Ns + i] = std::pair{ flip_idx_nn, 0.5 * interaction };

				// --------------------- KITAEV
				if (n_num == 0)
					localVal += (this->Kz + this->dKz(i)) * sisj;
				else if (n_num == 1)
					this->locEnergies[2 * this->Ns + i] = std::pair{ flip_idx_nn, -(this->Ky + this->dKy(i)) * sisj };
				else if (n_num == 2)
					this->locEnergies[3 * this->Ns + i] = std::pair{ flip_idx_nn, this->Kx + this->dKx(i) };
			}
		}
	}
	// append unchanged at the very end
	this->locEnergies[4 * this->Ns] = std::pair{ baseToInt(v), static_cast<_type>(localVal) };
}


// ----------------------------------------------------------------------------- BUILDING HAMILTONIAN -----------------------------------------------------------------------------

/*
* @brief Generates the total Hamiltonian of the system. The diagonal part is straightforward,
* while the non-diagonal terms need the specialized setHamiltonainElem(...) function
*/
template <typename _type>
void Heisenberg_kitaev<_type>::hamiltonian() {
	try {
		this->H = SpMat<_type>(this->N, this->N);										//  hamiltonian memory reservation
	}
	catch (const std::bad_alloc& e) {
		std::cout << "Memory exceeded" << e.what() << "\n";
		assert(false);
	}


	for (auto k = 0; k < this->N; k++) {
		for (int i = 0; i < this->Ns; i++) {
			// check all the neighbors
			auto nn_number = this->lattice->get_nn_number(i);
			
			// true - spin up, false - spin down
			double si = checkBit(k, this->Ns - i - 1) ? 1.0 : -1.0;

			// perpendicular field (SZ) - HEISENBERG
			this->H(k, k) += (this->h + this->dh(i)) * si;

			// HEISENBERG
			const u64 new_idx = flip(k, this->Ns - 1 - i);
			this->setHamiltonianElem(k, this->g + this->dg(i), new_idx);

			// check the correlations
			for (auto n_num = 0; n_num < nn_number; n_num++) {
				if (auto nn = this->lattice->get_nn(i, n_num); nn >= 0) { //   && nn > j
					// check Sz 
					double sj = checkBit(k, this->Ns - 1 - nn) ? 1.0 : -1.0;

					// --------------------- HEISENBERG 
					
					// diagonal elements setting  interaction field
					double interaction = (this->J + this->dJ(i));
					double sisj = si * sj;

					// setting the neighbors elements
					this->H(k, k) += interaction * this->delta * sisj;

					const u64 flip_idx_nn = flip(new_idx, this->Ns - 1 - nn);

					// S+S- + S-S+ hopping
					if (si * sj < 0)
						this->setHamiltonianElem(k, 0.5 * interaction, flip_idx_nn);
					
					// --------------------- KITAEV
					if (n_num == 0)
						this->setHamiltonianElem(k, (this->Kz + this->dKz(i)) * sisj, k);
					else if (n_num == 1)
						this->setHamiltonianElem(k, -(this->Ky + this->dKy(i)) * sisj, flip_idx_nn);
					else if (n_num == 2)
						this->setHamiltonianElem(k, this->Kx + this->dKx(i), flip_idx_nn);
				}
			}
		}
	}
}



#endif
