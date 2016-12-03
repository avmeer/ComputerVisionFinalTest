#include "Matrix.h"

#include <iomanip>
#include <sstream>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


	Matrix::Matrix() {
		setupMatrix( 4, 4 );
		set( 3, 3, 1 );
	}

	Matrix::Matrix( unsigned int rows, unsigned int cols ) {
		setupMatrix( rows, cols );
	}
	
	void Matrix::setupMatrix( unsigned int rows, unsigned int cols ) {
		_rows = rows;
		_cols = cols;

		for( unsigned int r = 0; r < getNumRows(); r++ ) {
			for( unsigned int c = 0; c < getNumCols(); c++ ) {
				_data.push_back(0.0);
			}
		}
	}
	
	Matrix Matrix::eye() {
		Matrix m;
		for( unsigned int i = 0; i < (getNumRows() < getNumCols() ? getNumRows() : getNumCols()); i++ )
			m.set(i,i,1);
		return m;
	}
		
	double Matrix::get( unsigned int row, unsigned int col ) {
		return _data[ col*getNumCols() + row ];
	}
	
	void Matrix::set( unsigned int row, unsigned int col, double val ) {
		_data[ col*getNumCols()+ row ] = val;
	}
	
	unsigned int Matrix::getNumRows() { return _rows; }
	unsigned int Matrix::getNumCols() { return _cols; }
	
	void Matrix::makeRotation( double theta, double x, double y, double z ) {
		double c = cos( theta );
		double s = sin( theta );
		
		set( 0, 0, x*x*(1-c)+c );
		set( 1, 0, x*y*(1-c)+z*s );
		set( 2, 0, x*z*(1-c)-y*s );
		
		set( 0, 1, x*y*(1-c)-z*s );
		set( 1, 1, y*y*(1-c)+c );
		set( 2, 1, y*z*(1-c)+x*s );
		
		set( 0, 2, x*z*(1-c)+y*s );
		set( 1, 2, y*z*(1-c)-x*s );
		set( 2, 2, z*z*(1-c)+c );
	}
	
	void Matrix::makeTranslation( double x, double y, double z ) {
		set( 0, 3, x );
		set( 1, 3, y );
		set( 2, 3, z );
	}
	
	Matrix Matrix::getSubMatrix( unsigned int rows, unsigned int cols ) {
		Matrix m( rows, cols );
		
		for( unsigned int r = 0; r < (rows < getNumRows() ? rows : getNumRows()); r++ ) {
			for( unsigned int c = 0; c < (cols < getNumCols() ? cols : getNumCols()); c++ ) {
				m.set( r, c, get(r,c) );
			}
		}
		
		return m;
	}
	
	void Matrix::transpose() {
		for( unsigned int r = 0; r < getNumRows(); r++ ) {
			for( unsigned int c = 0; c < getNumCols(); c++ ) {
				double temp = get(r,c);
				set( r, c, get(c,r) );
				set( c, r, temp );
			}
		}
	}
	
	double Matrix::determinate() {
		double det = 0;
		
		for( unsigned int c = 0; c < getNumCols(); c++ ) {
			double diag = 1;
			for( unsigned int r = 0; r < getNumRows(); r++ ) {
				diag *= get( r, (c+r)%getNumCols() );
			}
			det += diag;
		}
		
		for( int c = getNumCols()-1; c >= 0; c-- ) {
			double diag = 1;
			for( unsigned int r = 0; r < getNumRows(); r++ ) {
				diag *= get( r, (c-r)%getNumCols() );
			}
			det -= diag;
		}
		
		return det;
	}
	
	double* Matrix::asArray() {
		double* _vec = (double*)malloc( getNumRows()*getNumCols() * sizeof(double) );
		for( unsigned int i = 0; i < getNumRows()*getNumCols(); i++ ) {
			_vec[i] = _data[i];
		}
		return _vec;
	}
	
	const char* Matrix::toString() {
		std::stringstream ss;
		
		for( unsigned int r = 0; r < getNumRows(); r++ ) {
			ss << "[ ";
			for( unsigned int c = 0; c < getNumCols(); c++ ) {
				ss << std::setprecision(5) << std::fixed << std::setw(8);
				ss << get( r, c ) << " ";
			}
			ss << "]\n";
			ss << std::setw(0);
		}
		
		return ss.str().c_str();
	}
	
	Matrix operator*(float f, Matrix a) {
		Matrix m( a.getNumRows(), a.getNumCols() );
		for( unsigned int r = 0; r < a.getNumRows(); r++ ) {
			for( unsigned int c = 0; c < a.getNumCols(); c++ ) {
				m.set(r,c, f * a.get(r,c) );
			}
		}
		return m;
	}
	
	Matrix operator*(Matrix a, float f) {
		return f*a;
	}
	
	Matrix operator*(Matrix a, Matrix b) {
		assert( a.getNumCols() == b.getNumRows() );
		Matrix m( a.getNumRows(), b.getNumCols() );
		for( unsigned int r = 0; r < a.getNumRows(); r++ ) {
			for( unsigned int c = 0; c < b.getNumCols(); c++ ) {
				double sum = 0;
				for( unsigned int i = 0; i < b.getNumRows(); i++ ) {
					sum += a.get(r,i)*b.get(i,c);
				}
				m.set(r,c,sum);
			}
		}
		return m;
	}
	
	Matrix operator+(Matrix a, Matrix b) {
		assert( a.getNumRows() == b.getNumRows() && a.getNumCols() == b.getNumCols() );
		Matrix m( a.getNumRows(), a.getNumCols() );
		for( unsigned int r = 0; r < a.getNumRows(); r++ ) {
			for( unsigned int c = 0; c < a.getNumCols(); c++ ) {
				m.set(r,c, a.get(r,c) + b.get(r,c) );
			}
		}
		return m;
	}
