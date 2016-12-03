#include "Vector.h"

#include <assert.h>
#include <math.h>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif


	Vector::Vector() { x = y = z = w = 0; }

	Vector::Vector( int a, int b, int c ) { 
		x = a;
		y = b;
		z = c;
		w = 0;
	}

	Vector::Vector( double a, double b, double c ) { 
		x = a;
		y = b;
		z = c;
		w = 0;
	}

	// OPERATOR OVERLOADS

	Vector operator*(Vector a, Vector b) {
		return Vector( a.getX()*b.getX(), a.getY()*b.getY(), a.getZ()*b.getZ() );
	}

	Vector operator*(Vector a, float f) {
		return Vector(a.getX()*f,a.getY()*f,a.getZ()*f);
	}

	Vector operator/(Vector a, float f) {
		return Vector(a.getX()/f,a.getY()/f,a.getZ()/f);
	}

	Vector operator*(float f, Vector a) {
		return Vector(a.getX()*f,a.getY()*f,a.getZ()*f);
	}
	
	Vector operator*(Matrix m, Vector a) {
		assert( (m.getNumRows() == 3 || m.getNumRows() == 4) && m.getNumCols() == 4 );
		return Vector( m.get(0,0)*a.getX() + m.get(0,1)*a.getY() + m.get(0,2)*a.getZ() + m.get(0,3)*a.getW(),
					   m.get(1,0)*a.getX() + m.get(1,1)*a.getY() + m.get(1,2)*a.getZ() + m.get(1,3)*a.getW(),
					   m.get(2,0)*a.getX() + m.get(2,1)*a.getY() + m.get(2,2)*a.getZ() + m.get(2,3)*a.getW() );
	}

	Vector operator+(Vector a, Vector b) {
		return Vector(a.getX()+b.getX(), a.getY()+b.getY(), a.getZ()+b.getZ());
	}

	Vector operator-(Vector a, Vector b) {
		return Vector(a.getX()-b.getX(), a.getY()-b.getY(), a.getZ()-b.getZ());
	}


	Vector& Vector::operator+=(Vector rhs) {
		this->setX( this->getX() + rhs.getX() );
		this->setY( this->getY() + rhs.getY() );
		this->setZ( this->getZ() + rhs.getZ() );
		return *this;
	}


	Vector& Vector::operator-=(Vector rhs) {
		this->setX( this->getX() - rhs.getX() );
		this->setY( this->getY() - rhs.getY() );
		this->setZ( this->getZ() - rhs.getZ() );
		return *this;
	}

	Vector& Vector::operator*=(float rhs) {
		this->setX( this->getX() * rhs );
		this->setY( this->getY() * rhs );
		this->setZ( this->getZ() * rhs );
		return *this;
	}

	Vector& Vector::operator/=(float rhs) {
		this->setX( this->getX() / rhs );
		this->setY( this->getY() / rhs );
		this->setZ( this->getZ() / rhs );
		return *this;
	}

	bool operator==(Vector a, Vector b) {
		return ( a.getX() == b.getX() && a.getY() == b.getY() && a.getZ() == b.getZ() );
	}

	bool operator!=(Vector a, Vector b) {
		return !( a == b );
	}

	Vector cross(Vector a, Vector b) {
		Vector c;
		c.setX( a.getY()*b.getZ() - a.getZ()*b.getY() );
		c.setY( a.getZ()*b.getX() - a.getX()*b.getZ() );
		c.setZ( a.getX()*b.getY() - a.getY()*b.getX() );
		return c;
	}

	double dot(Vector a, Vector b) {
		return a.getX()*b.getX() + a.getY()*b.getY() + a.getZ()*b.getZ();
	}
	
	Matrix tensor(Vector a, Vector b) {
		Matrix m;
		
		for( unsigned int r = 0; r < 3; r++ ) {
			for( unsigned int c = 0; c < 3; c++ ) {
				m.set(r, c, a.get(r)*b.get(c));
			}
		}
		
		return m;
	}


	// MEMBER FUNCTIONS

	double Vector::magSq() {
		return x*x + y*y + z*z;
	}

	double Vector::mag() {
		double t = magSq();
		if(t <= 0.0)
			return 0;
		return sqrt(t);
	}

	void Vector::normalize() {
		double m = mag();
		x /= m;
		y /= m;
		z /= m;
	}

	double Vector::at(int i) {
		if(i == 0)  return x;
		if(i == 1)  return y;
		if(i == 2)  return z;
		return -1;
	}

	void Vector::glNormal() {
		glNormal3f( x, y, z );
	};

	Matrix Vector::crossProductMatrix() {
		Matrix m(4,4);
		m.set( 0, 1, -z );
		m.set( 1, 0,  z );
		m.set( 0, 2,  y );
		m.set( 2, 0, -y );
		m.set( 1, 2, -x );
		m.set( 2, 1,  x );
		return m;
	}
	

