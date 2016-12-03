#include "PointBase.h"

#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>


	double PointBase::getX() { return x; }

	void PointBase::setX( double x ) { this->x = x; }

	double PointBase::getY() { return y; }

	void PointBase::setY( double y ) { this->y = y; }

	double PointBase::getZ() { return z; }

	void PointBase::setZ( double z ) { this->z = z; }

	double PointBase::getW() { return w; }

	double PointBase::get( int i ) {
		switch( i ) {
		case 0:	return x;
		case 1:	return y;
		case 2:	return z;
		case 3:	return w;
		default:	return 0;
		}
	}
	
	double* PointBase::asArray() {
		return asArray4D();
	}
	
	/* deprecated */
	double* PointBase::asVector() {
		return asArray();
	}
	
	double* PointBase::asArray4D() {
		double* _vec = (double*)malloc( 4 * sizeof(double) );
		_vec[0] = x;
		_vec[1] = y;
		_vec[2] = z;
		_vec[3] = w;
		return _vec;
	}
	
	double* PointBase::asArray3D() {
		double* _vec = (double*)malloc( 3 * sizeof(double) );
		_vec[0] = x;
		_vec[1] = y;
		_vec[2] = z;
		return _vec;
	}
	
	double* PointBase::asArray2D() {
		double* _vec = (double*)malloc( 2 * sizeof(double) );
		_vec[0] = x;
		_vec[1] = y;
		return _vec;
	}
	
	double* PointBase::asArray1D() {
		double* _vec = (double*)malloc( 1 * sizeof(double) );
		_vec[0] = x;
		return _vec;
	}

	char* PointBase::toString() {
		char* buf = (char*)malloc( sizeof(char*) * 255 );
		sprintf( buf, "(%f, %f, %f, %f)", x, y, z, w );
		return buf;
	}


