#ifndef _MATRIX_H_
#define _MATRIX_H_ 1

#include <vector>



	class Matrix {
	public:
		
		Matrix();
		Matrix( unsigned int rows, unsigned int cols );
		
		Matrix eye();
		
		double get( unsigned int row, unsigned int col );
		void set( unsigned int row, unsigned int col, double val );
		
		unsigned int getNumRows();
		unsigned int getNumCols();
		
		void makeRotation( double theta, double x, double y, double z );
		void makeTranslation( double x, double y, double z );
		
		Matrix getSubMatrix( unsigned int rows, unsigned int cols );
		
		void transpose();
		
		double determinate();
				
		double* asArray();
		
		const char* toString();
		
	private:
		std::vector<double> _data;
		unsigned int _rows;
		unsigned int _cols;
		
		void setupMatrix( unsigned int rows, unsigned int cols );
	};

	Matrix operator*(float f, Matrix a);
	Matrix operator*(Matrix a, float f);
	Matrix operator*(Matrix a, Matrix b);
	Matrix operator+(Matrix a, Matrix b);	


#endif