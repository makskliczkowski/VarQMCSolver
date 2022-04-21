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
class IsingModelDis : public SpinHamiltonian<_type> {
private:
	// MODEL BASED PARAMETERS 
	double J = 1;																								// spin exchange
	double g = 1;																								// transverse magnetic field
	double h = 1;																								// perpendicular magnetic field

	vec dh;																										// disorder in the system - deviation from a constant h value
	double w;																									// the distorder strength to set dh in (-disorder_strength, disorder_strength)
	vec dJ;																										// disorder in the system - deviation from a constant J0 value
	double J0;																									// spin exchange coefficient
	vec dg;																										// disorder in the system - deviation from a constant g0 value
	double g0;																									// transverse magnetic field
public:
	/* Constructors */
	~IsingModelDis() = default;
	IsingModelDis() = default;
	IsingModelDis(double J, double J0, double g, double g0, double h, double w, std::shared_ptr<Lattice> lat);

private:
	u64 map(u64 index) override;

public:
	// METHODS
	void hamiltonian() override;
	v_1d<std::tuple<u64, _type>> locEnergy(u64 _id) override;													// returns the local energy for VQMC purposes
	v_1d<std::tuple<u64, _type>> locEnergy(const Col<_type>& _id) override;
	void setHamiltonianElem(u64 k, double value, u64 new_idx) override;

	static std::string set_info(int Ns, double J, double J0, double g, double g0, double h, double w,
	const v_1d<std::string>& skip = {}, std::string sep = "_") 
	{
		std::string name = sep + \
			"Ns=" + STR(Ns) + \
			",J=" + STRP(J, 2) + \
			",J0=" + STRP(J0, 2) + \
			",g=" + STRP(g, 2) + \
			",g0=" + STRP(g0, 2) + \
			",h=" + STRP(h, 2) + \
			",w=" + STRP(w, 2);
		auto tmp = split_str(name, ",");
		std::string tmp_str = "";
		for (int i = 0; i < tmp.size(); i++) {
			bool save = true;
			for (auto& skip_param : skip)
				// skip the element if we don't want it to be included in the info
				save = !(split_str(tmp[i], "=")[0] == skip_param);
			if (save) tmp_str += tmp[i] + ",";
		}
		tmp_str.pop_back();
		return tmp_str;
	}
};

// ----------------------------------------------------------------------------- CONSTRUCTORS -----------------------------------------------------------------------------

/*
* Ising disorder constructor
*/
template <typename _type>
IsingModelDis<_type>::IsingModelDis(double J, double J0, double g, double g0, double h, double w, std::shared_ptr<Lattice> lat)
	: J(J), g(g), h(h), w(w), J0(J0), g0(g0)

{
	this->lattice = lat;
	this->ran = randomGen();
	auto Ns = this->lattice->get_Ns();
	this->locEnergies = v_1d<std::tuple<u64, _type>>(Ns + 1);							// set local energies vector
	this->N = ULLPOW(Ns);
	this->dh = create_random_vec(Ns, this->ran, this->w);								// creates random disorder vector
	this->dJ = create_random_vec(Ns, this->ran, this->J0);								// creates random exchange vector
	this->dg = create_random_vec(Ns, this->ran, this->g0);								// creates random transverse field vector

	//change info
	this->info = this->set_info(this->lattice->get_Ns(), J, J0, g, g0, h, w);

}

// ----------------------------------------------------------------------------- BASE GENERATION AND RAPPING -----------------------------------------------------------------------------

/*
* Return the index in the case of no mapping in disorder
* index index to take
* @returns index
*/ 
template <typename _type>
u64 IsingModelDis<_type>::map(u64 index) {
	if (index < 0 || index >= std::pow(2, this->lattice->get_Ns())) throw "Element out of range\n No such index in map\n";
	return index;
}

// ----------------------------------------------------------------------------- LOCAL ENERGY -------------------------------------------------------------------------------------

/*
* Calculate the local energy end return the corresponding vectors with the value

*/
template <typename _type>
v_1d<std::tuple<u64,_type>> IsingModelDis<_type>::locEnergy(u64 _id) {
	auto Ns = this->lattice->get_Ns();
	const bool print = false;
	//stoutc(print) << "\t->" << VEQ(_id) << ":" << VEQ(tmp) << EL;


	// sumup the value of non-changed state
	double localVal = 0;
#pragma omp parallel for reduction(+ : localVal)
	for (auto i = 0; i < Ns; i++) {
		double sj=0;
		double si = checkBit(_id, Ns - i - 1) ? 1.0 : -1.0;								// true - spin up, false - spin down
		//stoutc(print) << "\t\t->" << VEQ(si) << EL;
		localVal += (this->h + dh(i)) * si;												// diagonal elements setting  perpendicular field
		// check the Siz Si+1z
		if(auto nei = this->lattice->get_nn(i, 0); nei >= 0)
			sj = checkBit(_id, Ns - 1 - nei) ? 1.0 : -1.0;
			//stoutc(print) << "\t\t->" << VEQ(sj) << EL;
			localVal += (this->J + this->dJ(i)) * si * sj;								// diagonal elements setting  interaction field
		// flip with S^x_i
		u64 new_id = flip(_id, BinaryPowers[Ns - 1 - i], Ns - 1 - i);					// flip to new state
		//stoutc(print) << "\t\t->" << VEQ(new_id) << EL << EL;

		this->locEnergies[i] = std::make_tuple(new_id, this->g + this->dg(i));			// get the new tuple
	}
	locEnergies[Ns] = std::make_tuple(_id, static_cast<_type>(localVal));				// append unchanged at the very end
	return this->locEnergies;
}





template<typename _type>
inline v_1d<std::tuple<u64, _type>> IsingModelDis<_type>::locEnergy(const Col<_type>& state)
{
	auto Ns = this->lattice->get_Ns();
	u64 _id = baseToInt(state, BinaryPowers);
	const bool print = false;

	v_1d<std::tuple<u64, _type>> elems(Ns + 1);
	// sumup the value of non-changed state
	_type localVal = 0;
	double si, sj = 0;
	for (auto i = 0; i < Ns; i++) {
		si = checkBit(_id, Ns - i - 1) ? 1.0 : -1.0;									// true - spin up, false - spin down
		stoutc(print) << "\t\t->" << VEQ(si) << EL;
		localVal += (this->h + dh(i)) * si;												// diagonal elements setting  perpendicular field
		// check the Siz Si+1z
		if (auto nei = this->lattice->get_nn(i, 0); nei >= 0)
			sj = checkBit(_id, Ns - 1 - nei) ? 1.0 : -1.0;
		stoutc(print) << "\t\t->" << VEQ(sj) << EL;
		localVal += (this->J + this->dJ(i)) * si * sj;									// diagonal elements setting  interaction field
		// flip with S^x_i
		u64 new_id = flip(_id, BinaryPowers[Ns - 1 - i], Ns - 1 - i);					// flip to new state
		stoutc(print) << "\t\t->" << VEQ(new_id) << EL << EL;

		elems[i] = std::make_tuple(new_id, this->g + this->dg(i));						// get the new tuple
	}
	elems[Ns] = std::make_tuple(_id, localVal);											// append unchanged at the very end
	return elems;
}


// ----------------------------------------------------------------------------- BUILDING HAMILTONIAN -----------------------------------------------------------------------------

/*
* Sets the non-diagonal elements of the Hamimltonian matrix, by acting with the operator on the k-th state
* @param k index of the basis state acted upon with the Hamiltonian 
* @param value value of the given matrix element to be set 
* @param temp resulting vector form acting with the Hamiltonian operator on the k-th basis state 
*/
template <typename _type>
void IsingModelDis<_type>::setHamiltonianElem(u64 k, double value, u64 new_idx) {
	this->H(new_idx, k) += value;
}

/*
* Generates the total Hamiltonian of the system. The diagonal part is straightforward,
* while the non-diagonal terms need the specialized setHamiltonainElem(...) function
*/
template <typename _type>
void IsingModelDis<_type>::hamiltonian() {
	auto Ns = this->lattice->get_Ns();
	try {
		this->H = SpMat<_type>(this->N, this->N);										//  hamiltonian memory reservation
	}
	catch (const std::bad_alloc& e) {
		std::cout << "Memory exceeded" << e.what() << "\n";
		assert(false);
	}

	for (long int k = 0; k < this->N; k++) {
		double s_i, s_j = 0;
		for (int j = 0; j <= Ns - 1; j++) {
			s_i = checkBit(k, Ns - 1 - j) ? 1.0 : -1.0;						// true - spin up, false - spin down

			NO_OVERFLOW(u64 new_idx = flip(k, BinaryPowers[Ns - 1 - j], Ns - 1 - j);)
				setHamiltonianElem(k, this->g + this->dg(j), new_idx);

			/* disorder */
			H(k, k) += (this->h + dh(j)) * s_i;                             // diagonal elements setting

			if (this->lattice->get_nn(j, 0) >= 0) {
				/* Ising-like spin correlation */
				s_j = checkBit(k, Ns - 1 - this->lattice->get_nn(j, 0)) ? 1.0 : -1.0;
				this->H(k, k) += (this->J + this->dJ(j)) * s_i * s_j;		// setting the neighbors elements
			}
		}
	}
}


#endif // !ISING_H