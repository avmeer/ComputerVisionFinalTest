#ifndef _GOL_MATERIAL_H_
#define _GOL_MATERIAL_H_ 1

#ifdef __APPLE__
        #include <OpenGL/glu.h>
#else
        #include <GL/glu.h>
#endif


	/* predefined materials */
	enum GOL_MaterialColor {
		GOL_MATERIAL_WHITE = 1,
		GOL_MATERIAL_BLACK = 2,
		GOL_MATERIAL_BRASS = 3,
		GOL_MATERIAL_REDPLASTIC = 4,
		GOL_MATERIAL_GREENPLASTIC = 5,
		GOL_MATERIAL_CYANRUBBER = 6
	};

	class Material {
	public:
		/* constructor */
		Material();
		/* use a predefined material color set */
		Material( GOL_MaterialColor preDefinedColor );
		
		/* get and set ambient component */
		GLfloat* getAmbient();
		void setAmbient( GLfloat amb[4] );
		
		/* get and set diffuse component */
		GLfloat* getDiffuse();
		void setDiffuse( GLfloat diff[4] );
		
		/* get and set specular component */
		GLfloat* getSpecular();
		void setSpecular( GLfloat spec[4] );
		
		/* get and set emissive componenet */
		GLfloat* getEmissive();
		void setEmissive( GLfloat emis[4] );
		
		/* get and set shininess value */
		GLfloat getShininess();
		void setShininess( GLfloat shin );
		
		/* get and set illumination type */
		/* used for compatibility with *.mtl files */
		GLint getIllumination();
		void setIllumination( GLint illum );
		
	private:
		GLfloat _ambient[4];
		GLfloat _diffuse[4];
		GLfloat _specular[4];
		GLfloat _emissive[4];
		GLfloat _shininess;
		GLint _illumination;
		
		void init( GOL_MaterialColor preDefinedColor );
	};

	/* set material to be the active material */
	void setCurrentMaterial( Material *material );


#endif
