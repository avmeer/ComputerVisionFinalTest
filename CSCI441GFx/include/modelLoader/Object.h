#ifndef _OPENGL_OBJECT_H_
#define _OPENGL_OBJECT_H_

#include "Face.h"
#include "Material.h"
#include "Point.h"

#include <map>
#include <string>
#include <vector>
using namespace std;


	
	class Object {
	public:
		Object();
		Object( string filename );
		~Object();
		
		bool loadObjectFile( string filename, bool INFO = true, bool ERRORS = true );
		
		bool draw();
		
		Point* getLocation();

		vector< Face* > *getFaces();
		vector< Point* > *getVertices();
		
	private:
		string _objFile;
		string _mtlFile;
		GLuint _objectDisplayList;
		
		bool objHasVertexTexCoords;
		bool objHasVertexNormals;
		
		void init();
		
		/* read in a WaveFront *.obj file */
		bool loadOBJFile( bool INFO = false, bool ERRORS = false );
		/* read in a WaveFront *.mtl file */
		bool loadMTLFile( bool INFO = false, bool ERRORS = false );
		
		/* read in a GEOMVIEW *.off file */
		bool loadOFFFile( bool INFO = false, bool ERRORS = false );

		/* read in a Stanford *.ply file */
		bool loadPLYFile( bool INFO = false, bool ERRORS = false );

		/* read in a STL *.stl file */
		bool loadSTLFile( bool INFO = false, bool ERRORS = false );

		vector< GLfloat > vertices;
		vector< GLfloat > vertexNormals;
		vector< GLfloat > vertexTexCoords;
		vector< GLfloat > vertexColors;
		
		map< string, Material* >* _materials;
		map< string, GLuint >* _textureHandles;

		Point* _location;
	};


#endif
