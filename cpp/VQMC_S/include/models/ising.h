#pragma once
#ifndef HAMIL_H
#include "../hamil.h"
#endif // !HAMIL_H


#ifndef ISINGMODEL
#define ISINGMODEL

/*
/// Model with disorder thus with no symmetries
*/
template <typename _type>
class IsingModel : public SpinHamiltonian<_type> {
private:
	// MODEL BASED PARAMETERS 
	double J = 1;																								// spin exchange
	double g = 1;																								// transverse magnetic field
	double h = 1;																								// perpendicular magnetic field

	vec tmp_vec;
	vec tmp_vec2;

	vec dh;																										// disorder in the system - deviation from a constant h value
	double w = 0;																								// the distorder strength to set dh in (-disorder_strength, disorder_strength)
	vec dJ;																										// disorder in the system - deviation from a constant J0 value
	double J0 = 0;																								// spin exchange coefficient
	vec dg;																										// disorder in the system - deviation from a constant g0 value
	double g0 = 0;																								// transverse magnetic field
public:
	// ------------------------------------------- 				 Constructors				  -------------------------------------------
	~IsingModel() = default;
	IsingModel() = default;
	IsingModel(double J, double J0, double g, double g0, double h, double w, std::shared_ptr<Lattice> lat);

private:
	u64 map(u64 index) override;

public:
	// METHODS
	void hamiltonian() override;
	void locEnergy(u64 _id) override;																			// returns the local energy for VQMC purposes
	void locEnergy(const vec& _id) override;																// returns the local energy for VQMC purposes
	void setHamiltonianElem(u64 k, _type value, u64 new_idx) override;											// sets the Hamiltonian elements

	string inf(const v_1d<string>& skip = {}, string sep = "_") const 
	{
		auto Ns = this->lattice->get_Ns();
		string name = sep + \
			"ising,Ns=" + STR(Ns) + \
			",J=" + STRP(J, 2) + \
			",J0=" + STRP(J0, 2) + \
			",g=" + STRP(g, 2) + \
			",g0=" + STRP(g0, 2) + \
			",h=" + STRP(h, 2) + \
			",w=" + STRP(w, 2);
		return SpinHamiltonian::inf(name, skip, sep);
	}
};

// ----------------------------------------------------------------------------- CONSTRUCTORS -----------------------------------------------------------------------------

/*
* @brief Ising disorder constructor
* @param J interaction between Sz's on the nearest neighbors
* @param J0 disorder at J interaction from (-J0,J0) added to J
* @param g transverse magnetic field
* @param g0 disorder at g field from (-g0, g0) added to g
* @param h perpendicular magnetic field 
* @param w disorder at h field from (-w, w) added to h
* @param lat general lattice class that informs about the topology of the system lattice
*/
template <typename _type>
IsingModel<_type>::IsingModel(double J, double J0, double g, double g0, double h, double w, std::shared_ptr<Lattice> lat)
	: J(J), g(g), h(h), w(w), J0(J0), g0(g0)
{
	this->lattice = lat;
	this->ran = randomGen();
	this->Ns = this->lattice->get_Ns();
	this->loc_states_num = this->Ns + 1;												// number of states after local energy work
	this->locEnergies = v_1d<std::pair<u64, _type>>(this->loc_states_num);				// set local energies vector
	this->N = ULLPOW(this->Ns);															// Hilber space size
	this->dh = create_random_vec(this->Ns, this->ran, this->w);							// creates random disorder vector
	this->dJ = create_random_vec(this->Ns, this->ran, this->J0);						// creates random exchange vector
	this->dg = create_random_vec(this->Ns, this->ran, this->g0);						// creates random transverse field vector

	//change info
	this->info = this->inf();

}

// ----------------------------------------------------------------------------- BASE GENERATION AND RAPPING -----------------------------------------------------------------------------

/*
* Return the index in the case of no mapping in disorder
* index index to take
* @returns index
*/ 
template <typename _type>
u64 IsingModel<_type>::map(u64 index) {
	if (index < 0 || index >= std::pow(2, this->lattice->get_Ns())) throw "Element out of range\n No such index in map\n";
	return index;
}

// ----------------------------------------------------------------------------- LOCAL ENERGY -------------------------------------------------------------------------------------

/*
* Calculate the local energy end return the corresponding vectors with the value
* @param _id base state index
*/
template <typename _type>
void IsingModel<_type>::locEnergy(u64 _id) {
	// sumup the value of a non-changed state
	double localVal = 0;
#pragma omp parallel for reduction(+ : localVal)
	for (auto i = 0; i < this->Ns; i++) {
		auto nn_number = this->lattice->get_nn_number(i);
		// true - spin up, false - spin down
		double si = checkBit(_id, this->Ns - i - 1) ? 1.0 : -1.0;								
		
		// diagonal elements setting the perpendicular field
		localVal += (this->h + dh(i)) * si;												
		
		// diagonal elements setting the interaction field
		for (auto n_num = 0; n_num < nn_number; n_num++) {
			if (auto nei = this->lattice->get_nn(i, n_num);  nei >= 0) { //  && nei >= i
				double sj = checkBit(_id, this->Ns - 1 - nei) ? 1.0 : -1.0;
				localVal += (this->J + this->dJ(i)) * si * sj;
			}
		}
		// flip with S^x_i with the transverse field
		u64 new_idx = flip(_id, this->Ns - 1 - i);
		this->locEnergies[i] = std::pair{ new_idx, this->g + this->dg(i) };
	}
	// append unchanged at the very end
	this->locEnergies[this->Ns] = std::pair{ _id, static_cast<_type>(localVal) };
}

/*
* Calculate the local energy end return the corresponding vectors with the value
* @param _id base state index
*/
template <typename _type>
void IsingModel<_type>::locEnergy(const vec& v) {
	// sumup the value of a non-changed state
	double localVal = 0;
#pragma omp parallel for reduction(+ : localVal)
	for (auto i = 0; i < this->Ns; i++) {
		auto nn_number = this->lattice->get_nn_number(i);

		// check Sz 
		double si = checkBitV(v, i) > 0 ? 1.0 : -1.0;

		// diagonal elements setting the perpendicular field
		localVal += (this->h + dh(i)) * si;

		// diagonal elements setting the interaction field
		for (auto n_num = 0; n_num < nn_number; n_num++) {
			if (auto nn = this->lattice->get_nn(i, n_num);  nn >= 0) { //  && nei >= i
				double sj = checkBitV(v, nn) > 0 ? 1.0 : -1.0;
				localVal += (this->J + this->dJ(i)) * si * sj;
			}
		}
		// flip with S^x_i with the transverse field
		this->tmp_vec = v;
		flipV(this->tmp_vec, i);
		const u64 new_idx = baseToInt(this->tmp_vec);
		this->locEnergies[i] = std::pair{ new_idx, this->g + this->dg(i) };
	}
	// append unchanged at the very end
	this->locEnergies[this->Ns] = std::pair{ baseToInt(v), static_cast<_type>(localVal) };
}

// ----------------------------------------------------------------------------- BUILDING HAMILTONIAN -----------------------------------------------------------------------------

/*
* Sets the non-diagonal elements of the Hamimltonian matrix, by acting with the operator on the k-th state
* @param k index of the basis state acted upon with the Hamiltonian 
* @param value value of the given matrix element to be set 
* @param new_idx resulting vector form acting with the Hamiltonian operator on the k-th basis state 
*/
template <typename _type>
void IsingModel<_type>::setHamiltonianElem(u64 k, _type value, u64 new_idx) {
	this->H(new_idx, k) += value;
}

/*
* Sets the non-diagonal elements of the Hamimltonian matrix, by acting with the operator on the k-th state
* @param k index of the basis state acted upon with the Hamiltonian
* @param value value of the given matrix element to be set
* @param new_idx resulting vector form acting with the Hamiltonian operator on the k-th basis state
*/
template <>
inline void IsingModel<cpx>::setHamiltonianElem(u64 k, cpx value, u64 new_idx) {
	this->H(new_idx, k) += value;
}

/*
* Generates the total Hamiltonian of the system. The diagonal part is straightforward,
* while the non-diagonal terms need the specialized setHamiltonainElem(...) function
*/
template <typename _type>
void IsingModel<_type>::hamiltonian() {
	try {
		this->H = SpMat<_type>(this->N, this->N);										//  hamiltonian memory reservation
	}
	catch (const std::bad_alloc& e) {
		std::cout << "Memory exceeded" << e.what() << "\n";
		assert(false);
	}

	for (long int k = 0; k < this->N; k++) {
		for (int j = 0; j <= this->Ns - 1; j++) {
			auto nn_number = this->lattice->get_nn_number(j);
			// true - spin up, false - spin down
			double s_i = checkBit(k, this->Ns - 1 - j) ? 1.0 : -1.0;							
			
			// flip with S^x_i with the transverse field
			u64 new_idx = flip(k, this->Ns - 1 - j);
			setHamiltonianElem(k, this->g + this->dg(j), new_idx);

			// diagonal elements setting the perpendicular field
			H(k, k) += (this->h + dh(j)) * s_i;											
			for (auto n_num = 0; n_num < nn_number; n_num++) {
				if (auto nn = this->lattice->get_nn(j, n_num); nn >= 0) {
					// Ising-like spin correlation
					double s_j = checkBit(k, this->Ns - 1 - nn) ? 1.0 : -1.0;
					// setting the neighbors elements
					this->H(k, k) += (this->J + this->dJ(j)) * s_i * s_j;
				}
			}
		}
	}
}


#endif // !ISING_H