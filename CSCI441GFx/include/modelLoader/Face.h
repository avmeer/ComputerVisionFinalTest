#ifndef _GOL_FACE_H_
#define _GOL_FACE_H_

#ifdef __APPLE__
        #include <OpenGL/glu.h>
#else
        #include <GL/glu.h>
#endif

#include "Material.h"
#include "Point.h"
#include "Vector.h"


	class Face {
	public:

		/* constructor */
		Face();
		
		/* get and set material */
		Material* getMaterial();
		void setMaterial( Material *mtl );
		
		/* get and set texture handle */
		GLuint getTextureHandle();
		void setTextureHandle( GLuint texHandle );
		
		/* get and set smooth shading */
		bool getSmooth();
		void setSmooth( bool s );
		
		/* get and set each point */
		Point getP();
		void setP( Point p );
		
		Point getQ();
		void setQ( Point q );
		
		Point getR();
		void setR( Point r );
		
		/* get and set each texture coordinate */
		/* only X and Y Point members are used */
		Point getPTexCoord();
		void setPTexCoord( Point pTex );
		
		Point getQTexCoord();
		void setQTexCoord( Point qTex );
		
		Point getRTexCoord();
		void setRTexCoord( Point rTex );
		
		/* get and set each vector normal */
		Vector getPNormal();
		void setPNormal( Vector pNorm );
		
		Vector getQNormal();
		void setQNormal( Vector qNorm );
		
		Vector getRNormal();
		void setRNormal( Vector rNorm );
		
		/* draw the face */
		/* frontFace by default */
		void draw( bool front = true );
		
		/* draw the front face */
		void drawFrontFace();
		/* draw the back face */
		void drawBackFace();
		
		/* calculate the center of mass of the face */
		Point CenterOfMass();
		
	private:
		Material *mtl;
		GLuint textureHandle;
		bool smooth;
		
		Point p, q, r;			// vertices
		Point pT, qT, rT;		// texture coords - only X and Y used
		Vector pN, qN, rN;		// normals
	};

#endif
