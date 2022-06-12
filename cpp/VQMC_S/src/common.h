#pragma once
/// user includes
#include "str.h"
#include "../include/random.h"

// --------------------------------------------------------				ARMA				--------------------------------------------------------

//-- SUPPRESS WARNINGS

#ifndef COMMON_H
#define COMMON_H


// armadillo flags:
#define ARMA_64BIT_WORD                                                                     // enabling 64 integers in armadillo obbjects
#define ARMA_BLAS_LONG_LONG                                                                 // using long long inside LAPACK call
#define ARMA_DONT_USE_FORTRAN_HIDDEN_ARGS
#define ARMA_DONT_USE_WRAPPER
#define ARMA_USE_MKL_ALLOC
#define ARMA_USE_MKL_TYPES
#define ARMA_USE_OPENMP
#define ARMA_ALLOW_FAKE_GCC
#include <armadillo>


#include <algorithm> 													// for std::ranges::copy depending on lib support
#include <iostream>
#include <ios>
#include <iomanip>
#include <thread>
#include <cmath>
#include <complex>
#include <cassert>
/// filesystem for directory creation
#ifdef __has_include
#  if __has_include(<filesystem>)
#    include <filesystem>
#    define have_filesystem 1
namespace fs = std::filesystem;
using clk = std::chrono::steady_clock;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
#    define have_filesystem 1
#    define experimental_filesystem
namespace fs = std::experimental::filesystem;
using clk = std::chrono::system_clock;
#  else
#    define have_filesystem 0
#  endif
#endif

static const char* kPSep =
#ifdef _WIN32
R"(\)";
#else
"/";
#endif


// --------------------------------------------------------				DEFINITIONS				--------------------------------------------------------

#define EL std::endl
#define stout std::cout << std::setprecision(8) << std::fixed											// standard out
#define stouts(text, start) stout << text << " -> time : " << tim_s(start) << "s" << EL					// standard out seconds
#define stoutms(text, start) stout << text << " -> time : " << tim_ms(start) << "ms" << EL				// standard out miliseconds
#define stoutmus(text, start) stout << text << " -> time : " << tim_mus(start) << "mus" << EL			// standard out microseconds
#define stoutc(c) if(c) stout <<  std::setprecision(8) << std::fixed									// standard out conditional
#define STR std::to_string
#define STRP(str,prec) str_p(str, prec)
#define DIAG arma::diagmat
#define VEQ(name) valueEquals(#name,(name),2)
#define VEQP(name,prec) valueEquals(#name,(name),prec)
#define EYE(X) arma::eye(X,X)
#define ZEROV(X) arma::zeros(X)
#define ZEROM(X) arma::zeros(X,X)
#define SPACE_VEC_D(Lx, Ly, Lz) v_3d<double>(Lx, v_2d<double>(Ly, v_1d<double>(Lz, 0)))
#define SPACE_VEC(Lx, Ly, Lz) v_3d<int>(Lx, v_2d<int>(Ly, v_1d<int>(Lz, 0)))
/// using types
using cpx = std::complex<double>;
using uint = unsigned int;
using ul = unsigned long;
using ull = unsigned long long;
using ld = long double;

/// constexpressions
constexpr long double PI = 3.141592653589793238462643383279502884L;			// it is me, pi
constexpr long double TWOPI = 2 * PI;										// it is me, 2pi
constexpr long double PI_half = PI / 2.0;									// it is me, half a pi
constexpr cpx imn = cpx(0, 1);												// complex number
const auto global_seed = std::random_device{}();							// global seed for classes
const std::string kPS = std::string(kPSep);
// --------------------------------------------------------				ALGORITHMS FOR MC				--------------------------------------------------------

/*
@brief Here we will state all the already implemented definitions that will help us building the user interfrace
*/
namespace impDef {
	/*
	/// Different Monte Carlo algorithms that can be provided inside the classes (for simplicity in enum form)
	*/
	enum class algMC {
		metropolis,
		heat_bath,
		self_learning
	};
	/*
	/// Types of implemented lattice types
	*/
	enum lattice_types {
		square,
		hexagonal
		//triangle,
		//hexagonal
	};

	enum ham_types {
		ising = 0,
		heisenberg = 1,
		heisenberg_dots = 2,
		kitaev_heisenberg = 3
	};
}

// --------------------------------------------------------				COMMON UTILITIES				 --------------------------------------------------------
using namespace arma;
using vecMat = v_1d<arma::mat>;
// -----------------------------------------------------------------------------				TIME FUNCTIONS				-----------------------------------------------------------------------------
/*
* return the duration in seconds from a given time point
* @param point in time from which we calculate the interval
*/
inline double tim_s(clk::time_point start) {
	return double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration(\
		std::chrono::high_resolution_clock::now() - start)).count()) / 1000.0;
}
/*
*/
inline double tim_ms(clk::time_point start) {
	return double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration(\
		std::chrono::high_resolution_clock::now() - start)).count());
}
/*
*/
inline double tim_mus(clk::time_point start) {
	return double(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration(\
		std::chrono::high_resolution_clock::now() - start)).count());
}

// debug definitions for compiler
#ifdef DEBUG
#define PRT(time_point, cond) stoutc(cond) << #cond << " -> time : " << tim_mus(time_point) << "mus" << EL;
#else
#define PRT(time_point, cond)
#endif


// -----------------------------------------------------------------------------				TOOLS				-----------------------------------------------------------------------------
//v_1d<double> fourierTransform(std::initializer_list<const arma::mat&> matToTransform, std::tuple<double,double,double> k, std::tuple<int,int,int> L);

// ----------------------------------------------------------------------------- MATRIX MULTIPLICATION
/*
* @brief Allows to calculate the matrix consisting of Column vector times row vector
* @param setMat matrix to set the elements onto
* @param setVec column vector to set the elements from
*/
template <typename _type>
inline void setColumnTimesRow(arma::Mat<_type>& setMat, const arma::Col<_type>& setVec) {
#pragma omp parallel for
	for (auto i = 0; i < setMat.n_rows; i++)
		for (auto j = 0; j < setMat.n_cols; j++)
			setMat(i, j) = conj(setVec(i)) * setVec(j);
}

/*
* @brief Allows to calculate the matrix consisting of Column vector times row vector but updates it instead of overwritng
* @param setMat matrix to set the elements onto
* @param setVec column vector to set the elements from
* @param plus if add or substract
*/
template <typename _type>
inline void setColumnTimesRow(arma::Mat<_type>& setMat, const arma::Col<_type>& setVec, bool plus) {
	if (plus)
#pragma omp parallel for
		for (auto i = 0; i < setMat.n_rows; i++)
			for (auto j = 0; j < setMat.n_cols; j++)
				setMat(i, j) += conj(setVec(i)) * setVec(j);
	else
#pragma omp parallel for
		for (auto i = 0; i < setMat.n_rows; i++)
			for (auto j = 0; j < setMat.n_cols; j++)
				setMat(i, j) -= conj(setVec(i)) * setVec(j);
}



/*
* @brief Allows to calculate the constant times column vector but updates it instead of overwritng
* @param setCol column to set the elements onto
* @param v value to multiply by
* @param multCol column vector to set the elements from
* @param conjug if we shall conjugate the column
*/
template <typename _type, typename _type2>
inline void setConstTimesCol(arma::Col<_type>& setCol, _type2 v, const arma::Col<_type>& multCol, bool conjug) {
	if(conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) = v * conj(multCol(i));
	else
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) = v * multCol(i);
}

/*
* @brief Allows to calculate the constant times column vector but updates it instead of overwritng
* @param setCol column to set the elements onto
* @param v value to multiply by
* @param multCol column vector to set the elements from
* @param conjug if we shall conjugate the column
*/
template <typename _type, typename _type2>
inline void setConstTimesCol(arma::Col<_type>& setCol, _type2 v, const arma::subview_col<_type>& multCol, bool conjug) {
	if (conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) = v * conj(multCol(i));
	else
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) = v * multCol(i);
}


/*
* @brief Allows to calculate the constant times column vector but updates it instead of overwritng
* @param setCol column to set the elements onto
* @param v value to multiply by
* @param multCol column vector to set the elements from
* @param plus if add or substract
* @param conjug if we shall conjugate the column
*/
template <typename _type, typename _type2>
inline void setConstTimesCol(arma::Col<_type>& setCol, _type2 v, const arma::Col<_type>& multCol, bool plus, bool conjug) {
	if (plus && conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) += v * conj(multCol(i));
	else if (plus && !conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) += v * multCol(i);
	else if (!plus && conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) -= v * conj(multCol(i));
	else
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) -= v * multCol(i);
}

/*
* @brief Allows to calculate the constant times column vector but updates it instead of overwritng
* @param setCol column to set the elements onto
* @param v value to multiply by
* @param multCol column vector to set the elements from
* @param plus if add or substract
* @param conjug if we shall conjugate the column
*/
template <typename _type, typename _type2>
inline void setConstTimesCol(arma::Col<_type>& setCol, _type2 v, const arma::subview_col<_type>& multCol, bool plus, bool conjug) {
	if (plus && conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) += v * conj(multCol(i));
	else if (plus && !conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) += v * multCol(i);
	else if (!plus && conjug)
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) -= v * conj(multCol(i));
	else
#pragma omp parallel for
		for (auto i = 0; i < setCol.n_elem; i++)
			setCol(i) -= v * multCol(i);
}




/*
* Puts the given matrix MSet(smaller) to a specific place in the M2Set (bigger) matrix
* @param M2Set (bigger) matrix to find the submatrix in and set it's elements
* @param MSet (smaller) matrix to be put in the M2Set
* @param row row of the left upper element (row,col) of M2Set
* @param col col of the left upper element (row,col) of M2Set
* @param update if we shall add or substract MSet elements from M2Set depending on minus parameter
* @param minus substract?
*/
void setSubmatrixFromMatrix(arma::mat& M2Set, const arma::mat& MSet, uint row, uint col, uint Nrows, uint Ncols, bool update = true, bool minus = false);

/*
* @brief Uses the given matrix MSet (bigger) to set the M2Set (smaller) matrix
* @param M2Set (smaller) matrix to find the submatrix in and set it's elements
* @param MSet (bigger) matrix to be put in the M2Set
* @param row row of the left upper element (row,col) of MSet
* @param col col of the left upper element (row,col) of MSet
* @param update if we shall add or substract MSet elements from M2Set depending on minus parameter
* @param minus substract?
*/
void setMatrixFromSubmatrix(arma::mat& M2Set, const arma::mat& MSet, uint row, uint col, uint Nrows, uint Ncols, bool update = true, bool minus = false);

/*
* @brief Is used to calculate the equation of the form (U_l * D_l * T_l + U_r * D_r * T_r).
* @details UDT we get from QR decomposition with column pivoting
* @param Ql
* @param Rl
* @param Pl
* @param Tl
* @param Dl
* @param Qr
* @param Rr
* @param Pr
* @param Tr
* @param Dr
* @param Dtmp
* @warning Uses the UDT decomposition from QR with column pivoting
*/
arma::mat inv_left_plus_right_qr(arma::mat& Ql, arma::mat& Rl, arma::umat& Pl, arma::mat& Tl, arma::vec& Dl, arma::mat& Qr, arma::mat& Rr, arma::umat& Pr, arma::mat& Tr, arma::vec& Dr, arma::vec& Dtmp);

/*
* @brief Creates the UDT decomposition using QR decomposition. WITH INVERSION OF R DIAGONAL ALREADY
* @cite doi:10.1016/j.laa.2010.06.023
* @param mat 
* @param Q unitary Q matrix
* @param R right triangular matrix
* @param P permutation matrix
* @param T upper triangular matrix
* @param D diagonal vector -> saves the inverse already
*/
void inline setUDTDecomp(const arma::mat& mat, arma::mat& Q, arma::mat& R, arma::umat& P, arma::mat& T, arma::vec& D) {
	if (!arma::qr(Q, R, P, mat)) throw "decomposition failed\n";
	// inverse during setting
	for (int i = 0; i < R.n_rows; i++)
		D(i) = 1.0 / R(i, i);
	T = ((DIAG(D) * R) * P.t());
}

/*
* @brief Creates the UDT decomposition using QR decomposition. WITHOUT D VECTOR
* @cite doi:10.1016/j.laa.2010.06.023
* @param mat 
* @param Q unitary Q matrix
* @param R right triangular matrix
* @param P permutation matrix
* @param T upper triangular matrix
* @param D diagonal vector -> saves the inverse already
*/
void inline setUDTDecomp(const arma::mat& mat, arma::mat& Q, arma::mat& R, arma::umat& P, arma::mat& T) {
	if (!arma::qr(Q, R, P, mat)) throw "decomposition failed\n";
	// inverse during setting
	T = ((arma::inv(DIAG(R)) * R) * P.t());
}


/**
 * @brief Calculate the multiplication of two matrices with numerical stability.
 * @param right right matrix of multiplication
 * @param left left matrix of multiplication
 * @param Ql 
 * @param Rl 
 * @param Pl 
 * @param Tl 
 * @param Qr 
 * @param Rr 
 * @param Pr 
 * @param Tr 
 * @return matrix after multiplication
 */
arma::mat inline stableMultiplication(const arma::mat& left, const arma::mat& right,
									arma::mat& Ql, arma::mat& Rl, arma::umat& Pl, arma::mat& Tl,
									arma::mat& Qr, arma::mat& Rr, arma::umat& Pr, arma::mat& Tr
									)
{
	const auto type = 0; // SVD
	//const auto type = 'QR';
	if (type == 1) {
		setUDTDecomp(left, Ql, Rl, Pl, Tl);
		setUDTDecomp(right, Qr, Rr, Pr, Tr);
		setUDTDecomp(DIAG(Rl) * ((Tl * Qr) * DIAG(Rr)), Qr, Rr, Pr, Tl);
		return (Ql * Qr) * DIAG(Rr) * (Tl * Tr);
	}
	//else if(type == 'SVD'){
	//	svd(Ql, DIAG(Rl), mat V, mat X)
	//}
	else return left * right;
}


/*
* @brief Using ASvQRD - Accurate Solution via QRD with column pivoting to multiply the QR on the right and multiply new matrix mat_to_multiply on the left side.
* @cite doi:10.1016/j.laa.2010.06.023
* @param mat_to_multiply (left) matrix to multiply by the QR decomposed stuff (on the right)
* @param Q unitary Q matrix
* @param R right triangular matrix
* @param P permutation matrix
* @param T upper triangular matrix
* @param D inverse of the new R diagonal
*/
void inline multiplyMatricesQrFromRight(const arma::mat& mat_to_multiply, arma::mat& Q, arma::mat& R, arma::umat& P, arma::mat& T, arma::vec& D) {
	if (!arma::qr(Q, R, P, (mat_to_multiply * Q) * arma::diagmat(R))) throw "decomposition failed\n";
	// inverse during setting
	for (int i = 0; i < R.n_rows; i++)
		D(i) = 1.0 / R(i, i);
	// premultiply old T by new T from left
	T = ((DIAG(D) * R) * P.t()) * T;
}


void inline multiplyMatricesSVDFromRight(const arma::mat& mat_to_multiply, arma::mat& U, arma::vec& s, arma::mat& V, arma::mat& tmpV) {
	svd(U, s, tmpV, mat_to_multiply * U * DIAG(s));
	V = V * tmpV;
}
/*
* @brief Loh's decomposition to two scales in UDT QR decomposition. One is lower than 0 and second higher. Uses R again to save memory
* @param R the R matrix from QR decompositon. As it's diagonal is mostly not used anymore it will be used to store (<= 1) elements of previous R
* @param D vector to store (> 1) elements of previous R -> IT IS ALREADY INVERSE OF R DIAGONAL
*/
void inline makeTwoScalesFromUDT(arma::mat& R, arma::vec& D) {
	for (int i = 0; i < R.n_rows; i++)
	{
		if (abs(R(i, i)) > 1)
			R(i, i) = 1;				// min(1,R(i,i))
		else
			D(i) = 1;					// inv of max(1,R(i,i))
	}
}

/*
* @brief Loh's decomposition to two scales in UDT QR decomposition. One is lower than 0 and second higher. Uses two new vectors
* @param R the R matrix from QR decompositon. As it's diagonal is mostly not used anymore it will be used to store (<= 1) elements of previous R
* @param D vector to store (> 1) elements of previous R
*/
void inline makeTwoScalesFromUDT(const arma::mat& R, arma::vec& Db, arma::vec& Ds) {
	Db.ones();
	Ds.ones();
	for (int i = 0; i < R.n_rows; i++)
	{
		if (abs(R(i, i)) > 1)
			Db(i) = R(i,i);
		else
			Ds(i) = R(i,i);
	}
}

//! ----------------------------------------------------------------------------- FILE AND STREAMS



/*
* @brief Opens a file
* @param filename filename
* @param mode std::ios_base::openmode
*/
template <typename T>
inline void openFile(T& file, std::string filename, std::ios_base::openmode mode = std::ios::out) {
	file.open(filename, mode);
	if (!file.is_open()) {
		stout << "couldn't open a file: " + filename + "\n";
		throw "couldn't open a file: " + filename + "\n";
	}
}



// ----------------------------------------------------------------------------- DIRECTORIES -----------------------------------------------------------------------------

/*
* @brief Creates a single directory given a string path
* @param dir the directory
*/
inline void createDirs(const std::string& dir) {
	fs::create_directories(dir);
}

/*
* @brief Creates a variadic directory set given a string paths
* @param dir the directory
*/
template <typename... _Ty>
inline void createDirs(const std::string& dir, const _Ty&... dirs) {
	createDirs(dir);
	createDirs(dirs...);
}


//? ------------------------------------------------------------------------------ VALUE EQUALS ------------------------------------------------------------------------------
/*
* checks if value is equal to some param up to given tolerance
*/
template <typename T> 
inline bool valueEqualsPrec(T value, T eq, T tol) {
	return std::abs(value - eq) < tol;
}


/*
*Changes a value to a string with a given precision
*@param a_value Value to be transformed
*@param n Precision @n default 2
*@returns String of a value
*/
template <typename T>
inline std::string str_p(const T a_value, const int n = 2) {
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}

/*
* @brief pretty prints the complex number in angular form
* @param val complex value
* @n precision
*/
inline std::string print_cpx(cpx val, int n = 2) {
	double phase = std::arg(val) / PI;
	while (phase < 0)
		phase += 2;
	auto absolute = "+" + str_p(std::abs(val), n);
	std::string phase_str = "";
	if (valueEqualsPrec(phase, 0.0, 1e-3) || valueEqualsPrec(phase, 2.0, 1e-3))
		phase_str = "";
	else if (valueEqualsPrec(phase, 1.0, 1e-3)) {
		absolute = "-" + str_p(std::abs(val), n);;
		phase_str = "";
	}
	else {
		phase_str = "*exp(" + str_p(phase, n) + "*pi*i)";
	}


	return absolute + phase_str;
}

/*
* given the char* name it prints its value in a format "name=val"
*@param name name of the variable
*@param value of the variable
*@returns "name=val" string
*/
template <typename T>
inline std::string valueEquals(const char name[], T value, int prec = 2) {
	return std::string(name)+ "=" + str_p(value, prec);
}


/*
* given the char* name it prints its value in a format "name=val" specialization for string
*@param name name of the variable
*@param value of the variable
*@returns "name=val" string
*/
inline std::string valueEquals(const char name[], std::string value, int prec) {
	return std::string(name) + "=" + value;
}

// -------------------------------------------------------------------------------------------- PRINT SEPARATED -----------------------------------------------------------------------------

/*
* printing the separated number of variables using the variadic functions initializer
*@param output output stream
*@param elements initializer list of the elements to be printed
*@param separator to be used @n default "\\t"
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*/
template <typename T>
inline void printSeparated(std::ostream& output, char separtator = '\t', std::initializer_list<T> elements = {}, arma::u16 width = 8, bool endline = true) {
	for (auto elem : elements) {
		output.width(width); output << elem << std::string(1,separtator);
	}
	if (endline) output << std::endl;
}


/*
* printing the separated number of variables using the variadic functions initializer -> ONE TYPE FUNCTION FOR RECURSION
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param elements at the very end we give any type of variable to the function
*/
template <typename Type>
inline void printSep(std::ostream& output, char separator, arma::u16 width, Type arg) {
	output.width(width); output << arg << std::string(1, separator);
}
/*
* printing the separated number of variables using the variadic functions initializer -> ONE TYPE FUNCTION FOR RECURSION - PRECISION!
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param elements at the very end we give any type of variable to the function
*@param prec precision for the output
*/
template <typename Type>
inline void printSepP(std::ostream& output, char separator, arma::u16 width, u16 prec, Type arg) {
	output.width(width); output << str_p(arg,prec) << std::string(1, separator);
}

/*
* printing the separated number of variables using the variadic functions initializer
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param arg first element of the argument list
*@param elements at the very end we give any type of variable to the function
*/
template <typename Type, typename... Types>
inline void printSep(std::ostream& output, char separator, arma::u16 width, Type arg, Types... elements) {
	printSep(output, separator, width, arg);
	printSep(output, separator, width, elements...);
}

/*
* printing the separated number of variables using the variadic functions initializer PRECISION
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param arg first element of the argument list
*@param elements at the very end we give any type of variable to the function
*/
template <typename Type, typename... Types>
inline void printSepP(std::ostream& output, char separator, arma::u16 width, u16 prec, Type arg, Types... elements) {
	printSepP(output, separator, width, prec, arg);
	printSepP(output, separator, width, prec, elements...);
}

/*
* printing the separated number of variables using the variadic functions initializer - LAST CALL
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param elements at the very end we give any type of variable to the function
*/
template <typename... Types>
inline void printSeparated(std::ostream& output, char separator, arma::u16 width, bool endline, Types... elements) {
	printSep(output, separator, width, elements...);
	if (endline) output << std::endl;
}

/*
*@brief printing the separated number of variables using the variadic functions initializer - LAST CALL PRECISION
*@param output output stream
*@param separator to be used
*@param width of one element column for printing
*@param endline shall we add endline at the end?
*@param elements at the very end we give any type of variable to the function
*/
template <typename... Types>
inline void printSeparatedP(std::ostream& output, char separator, arma::u16 width, bool endline, u16 prec, Types... elements) {
	printSepP(output, separator, width, prec, elements...);
	if (endline) output << std::endl;
}

// ---------------------------------------------------------------------------------------- STREAM OVERLOADED

/*
*Overwritten standard stream redirection operator for 2D vectors separated by commas
*@param out outstream to be used
*@param v 1D vector
*/
template <typename T>
std::ostream& operator<< (std::ostream& out, const v_1d<T>& v) {
	if (!v.empty()) {
		//out << '[';
		for (int i = 0; i < v.size(); i++)
			out << v[i] << ",";
		out << "\b"; // use two ANSI backspace characters '\b' to overwrite final ", "
	}
	return out;
}

/*
* @brief Overwritten standard stream redirection operator for 2D vectors
* @param out outstream to be used
* @param v 2D vector
*/
template <typename T>
std::ostream& operator << (std::ostream& out, const v_2d<T>& v ) {
	if (!v.empty()) {
		for (auto it : v) {
			out << "\t\t\t\t";
			for (int i = 0; i < it.size(); i++)
				out << it[i] << '\t';
			out << "\n";
		}
	}
	return out;
}
//! ----------------------------------------------------------------------------- HELPERS -----------------------------------------------------------------------------

/*
* @brief check the sign of a value
* @param val value to be checked
* @returns sign of a variable
*/
template <typename T>
inline int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

/*
* @brief Defines an euclidean modulo denoting also the negative sign
* @param a left side of modulo
* @param b right side of modulo
* @returns euclidean a%b
* @link https://en.wikipedia.org/wiki/Modulo_operation
*/
inline int myModuloEuclidean(int a, int b)
{
	int m = a % b;
	if (m < 0) m = (b < 0) ? m - b : m + b;
	return m;
}


// ----------------------------------------------------------------------------- VECTORS HANDLING -----------------------------------------------------------------------------




template <typename T, typename T2>
inline void print_vector_1d(T& file, const v_1d<T2>& v) {
	for (auto i = 0; i < v.size(); i++)
		printSeparatedP(file, '\t', 8, true, 5, i, v[i]);
}

template <typename T, typename T2>
inline void print_vector_2d(T& file, const v_2d<T2>& v) {
	for (auto i = 0; i < v.size(); i++)
		print_vector_1d(file, v[i]);
}

template <typename T, typename T2>
inline void print_vector_3d(T& file, const v_3d<T2>& v) {
	//for (auto i = 0; i < v.size(); i++)
	//	print_vector_2d(file, v[i]);
	for(auto i = 0; i < v.size(); i++)
		for(auto j = 0; j < v[i].size(); j++)
			for(auto k = 0; k < v[i][j].size(); k++)
				printSeparatedP(file, '\t', 8, true, 5, i, j, k, v[i][j][k]);
}

template <typename T, typename T2>
inline void print_mat(T& file, const Mat<T2>& m) {
	//for (auto i = 0; i < v.size(); i++)
	//	print_vector_2d(file, v[i]);
	for (auto i = 0; i < m.n_rows; i++)
		for (auto j = 0; j < m.n_cols; j++)
				printSeparatedP(file, '\t', 8, true, 5, i, j, m(i,j));
}



template <typename T, typename T2>
inline void print_vector_1d(T& file, const Col<T2>& v) {
	for (auto i = 0; i < v.size(); i++)
		printSeparatedP(file, '\t', 8, true, 5, i, v(i));
}

/*
* @brief take real part of a complex vector
*/
//v_1d<double> realv(const v_1d<cpx>& v) {
//	v_1d<double> tmp(v.size(), 0);
//	for (auto i = 0; i < v.size(); i++)
//		tmp[i] = real(v[i]);
//	return tmp;
//}
//
//
///*
//* @brief take imaginary part of a complex vector
//*/
//v_1d<double> imagv(const v_1d<cpx>& v) {
//	v_1d<double> tmp(v.size(), 0);
//	for (auto i = 0; i < v.size(); i++)
//		tmp[i] = imag(v[i]);
//	return tmp;
//}

/*
* 
*/
template <typename T>
T stddev(const v_1d<T>& v)
{
	T sum = std::accumulate(v.begin(), v.end(), cpx(0.0));
	T mean = sum / cpx(v.size());

	std::vector<T> diff(v.size());
	std::ranges::transform(v.begin(), v.end(), diff.begin(), [mean](T x) { return x - mean; });
	T sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), cpx(0.0));
	T stdev = std::sqrt(sq_sum / cpx(v.size()));
	return stdev;
}


/// <summary>
/// Creates a random vector of custom length using the random library and the merson-twister (?) engine
/// </summary>
/// <param name="N"> length of the generated random vector </param>
/// <returns> returns the custom-length random vector </returns>
inline vec create_random_vec(u64 N, randomGen& gen, double h = 1.0) {
	vec random_vec(N, fill::zeros);
	// create random vector from middle to always append new disorder at lattice endpoint
	for (u64 j = 0; j <= N / 2.; j++) {
		u64 idx = N / (long)2 - j;
		random_vec(idx) = gen.randomReal_uni(-h, h);
		idx += 2 * j;
		if (idx < N) random_vec(idx) = gen.randomReal_uni(-h, h);
	}
	return random_vec;
}

/// <summary>
/// Creates a random vector of custom length using the random library and the merson-twister (?) engine
/// </summary>
/// <param name="N"> length of the generated random vector </param>
/// <returns> returns the custom-length random vector </returns>
inline std::vector<double> create_random_vec_std(u64 N, randomGen& gen, double h = 1.0) {
	std::vector<double> random_vec(N, 0);
	for (u64 j = 0; j < N; j++) {
		random_vec[j] = gen.randomReal_uni(-h, h);
	}
	return random_vec;
}

#endif // !COMMON_H
