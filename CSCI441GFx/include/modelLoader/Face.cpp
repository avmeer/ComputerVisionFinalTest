#include "Face.h"

#include <stdlib.h>


	
	Face::Face() {
		mtl = NULL;
		textureHandle = 0;
		smooth = true;
	}
	
	/* get and set material */
	Material* Face::getMaterial() { return this->mtl; }
	void Face::setMaterial( Material *mtl ) { this->mtl = mtl; }
	
	/* get and set texture handle */
	GLuint Face::getTextureHandle() { return textureHandle; }
	void Face::setTextureHandle( GLuint texHandle ) { textureHandle = texHandle; }
	
	/* get and set smooth shading */
	bool Face::getSmooth() { return smooth; }
	void Face::setSmooth( bool s ) { smooth = s; }
	
	/* get and set each point */
	Point Face::getP() { return p; }
	void Face::setP( Point p ) { this->p = p; }
	
	Point Face::getQ() { return q; }
	void Face::setQ( Point q ) { this->q = q; }
	
	Point Face::getR() { return r; }
	void Face::setR( Point r ) { this->r = r; }
	
	/* get and set each texture coordinate */
	/* only X and Y Point members are used */
	Point Face::getPTexCoord() { return pT; }
	void Face::setPTexCoord( Point pTex ) { pT = pTex; }
	
	Point Face::getQTexCoord() { return qT; }
	void Face::setQTexCoord( Point qTex ) { qT = qTex; }
	
	Point Face::getRTexCoord() { return rT; }
	void Face::setRTexCoord( Point rTex ) { rT = rTex; }
	
	/* get and set each vector normal */
	Vector Face::getPNormal() { return pN; }
	void Face::setPNormal( Vector pNorm ) { pN = pNorm; }
	
	Vector Face::getQNormal() { return qN; }
	void Face::setQNormal( Vector qNorm ) { qN = qNorm; }
	
	Vector Face::getRNormal() { return rN; }
	void Face::setRNormal( Vector rNorm ) { rN = rNorm; }

	void Face::draw( bool front ) {
		if( smooth ) 
			glShadeModel( GL_SMOOTH );
		else 
			glShadeModel( GL_FLAT );
		
		if( mtl != NULL ) {
			setCurrentMaterial( mtl );
		}
		
		if( textureHandle != 0 ) {
			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, textureHandle );
		} else {
			glDisable( GL_TEXTURE_2D );
		}
		
		glBegin( GL_TRIANGLES ); {
			(front ? pN.glNormal() : (-1 * pN).glNormal());	
			if( textureHandle != 0 ) pT.glTexCoord();
			p.glVertex();
			
			(front ? qN.glNormal() : (-1 * qN).glNormal());		
			if( textureHandle != 0 ) qT.glTexCoord();
			q.glVertex();
			
			(front ? rN.glNormal() : (-1 * rN).glNormal());	
			if( textureHandle != 0 ) rT.glTexCoord();
			r.glVertex();
		}; glEnd();
		
		glDisable( GL_TEXTURE_2D );	
	}

	void Face::drawFrontFace() { draw( true ); }
	void Face::drawBackFace() { draw( false ); }

	Point Face::CenterOfMass() {
		return (p + q + r) / 3;
	}


