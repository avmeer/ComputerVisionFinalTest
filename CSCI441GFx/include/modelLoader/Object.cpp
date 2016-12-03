#ifdef __APPLE__
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif

#include <SOIL/soil.h>

#include "Object.h"
#include "Point.h"
#include "Vector.h"

#include <fstream>
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <time.h>

	vector<string> tokenizeString(string input, string delimiters);
	unsigned char* createTransparentTexture( unsigned char *imageData, unsigned char *imageMask, int texWidth, int texHeight, int texChannels, int maskChannels );
	unsigned char* loadPPM( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path );
	unsigned char* loadBMP( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path );
	unsigned char* loadTGA( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path );

	Object::Object() {
		init();
	}

	Object::Object( string filename ) : _objFile( filename ) {
		init();
		loadObjectFile( filename );
	}

	Object::~Object() {
		delete _materials;
		delete _textureHandles;
	}

	bool Object::loadObjectFile( string filename, bool INFO, bool ERRORS ) {
		bool result = true;
		_objFile = filename;
		if( filename.find( ".obj" ) != string::npos ) {
			result = loadOBJFile( INFO, ERRORS );
		} else if( filename.find( ".off" ) != string::npos ) {
			result = loadOFFFile( INFO, ERRORS );
		} else if( filename.find( ".ply" ) != string::npos ) {
			result = loadPLYFile( INFO, ERRORS );
		} else if( filename.find( ".stl" ) != string::npos ) {
			result = loadSTLFile( INFO, ERRORS );
		}
		else {
			result = false;
			if (ERRORS) cout << "[.OBJ]: [ERROR]:  Unsupported file format for file: " << filename << endl;
		}

		return result;
	}

	bool Object::draw() {
		bool result = true;
		
		glPushMatrix(); {
			glCallList( _objectDisplayList );
		}; glPopMatrix();
		
		return result;
	}

	Point* Object::getLocation() { 
		return _location;
	}

	vector< Face* >* Object::getFaces() {
		vector< Face* > *faces = new vector< Face* >();
		
		delete _materials;
		delete _textureHandles;
		
		_materials = new map< string, Material* >();
		_textureHandles = new map< string, GLuint >();
		
		Material *materialForFace = NULL;
		GLuint textureForFace = 0;
		bool smoothForFace = true;
		
		ifstream in( _objFile.c_str() );
		string line;
		while( getline( in, line ) ) {
			line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
			
			vector< string > tokens = tokenizeString( line, " " );
			if( tokens.size() < 1 ) continue;
			
			//the line should have a single character that lets us know if it's a...
			if( !tokens[0].compare( "#" ) ) {								// comment ignore
			} else if( !tokens[0].compare( "o" ) ) {						// object name ignore
			} else if( !tokens[0].compare( "g" ) ) {						// polygon group name ignore
			} else if( !tokens[0].compare( "mtllib" ) ) {					// material library
				loadMTLFile();
			} else if( !tokens[0].compare( "usemtl" ) ) {					// use material library
				map< string, Material* >::iterator materialIter = _materials->find( tokens[1] );
				if( materialIter != _materials->end() ) {
					materialForFace = materialIter->second;
				} else {
					materialForFace = NULL;
				}
				
				map< string, GLuint >::iterator textureIter = _textureHandles->find( tokens[1] );
				if( textureIter != _textureHandles->end() ) {
					textureForFace = textureIter->second;
				} else {
					textureForFace = 0;
				}
			} else if( !tokens[0].compare( "s" ) ) {						// smooth shading
				if( !tokens[1].compare( "off" ) ) {
					smoothForFace = false;
				} else {
					smoothForFace = true;
				}
			} else if( !tokens[0].compare( "v" ) ) {						//vertex
			} else if( !tokens.at(0).compare( "vn" ) ) {                    //vertex normal
			} else if( !tokens.at(0).compare( "vt" ) ) {                    //vertex tex coord
			} else if( !tokens.at(0).compare( "f" ) ) {                     //face!
				
				//now, faces can be either quads or triangles (or maybe more?)
				//split the string on spaces to get the number of verts+attrs.
				vector<string> faceTokens = tokenizeString(line, " ");
				
				//some local variables to hold the vertex+attribute indices we read in.
				//we do it this way because we'll have to split quads into triangles ourselves.
				vector<int> v, vn, vt;
				
				bool faceHasVertexTexCoords = false, faceHasVertexNormals = false;
				
				for(long unsigned int i = 1; i < faceTokens.size(); i++) {
					//need to use both the tokens and number of slashes to determine what info is there.
					vector<string> groupTokens = tokenizeString(faceTokens[i], "/");
					int numSlashes = 0;
					for(long unsigned int j = 0; j < faceTokens[i].length(); j++) { if(faceTokens[i][j] == '/') numSlashes++; }
					
					int vert = atoi(groupTokens[0].c_str());
					if( vert < 0 )
						vert = (vertices.size() / 3) + vert + 1;
					
					//regardless, we always get a vertex index.
					v.push_back( vert - 1 );
					
					//based on combination of number of tokens and slashes, we can determine what we have.
					if(groupTokens.size() == 2 && numSlashes == 1) {
						int vtI = atoi(groupTokens[1].c_str());	
						if( vtI < 0 )
							vtI = (vertexTexCoords.size() / 2) + vtI + 1;
						
						vtI--;
						vt.push_back( vtI ); 
						objHasVertexTexCoords = true; 
						faceHasVertexTexCoords = true;
					} else if(groupTokens.size() == 2 && numSlashes == 2) {
						int vnI = atoi(groupTokens[1].c_str());
						if( vnI < 0 )
							vnI = vertexNormals.size() + vnI + 1;
						
						vnI--;
						vn.push_back( vnI ); 
						objHasVertexNormals = true; 
						faceHasVertexNormals = true;
					} else if(groupTokens.size() == 3) {
						int vtI = atoi(groupTokens[1].c_str());
						if( vtI < 0 )
							vtI = vertexTexCoords.size() + vtI + 1;
						
						vtI--;
						vt.push_back( vtI ); 
						objHasVertexTexCoords = true; 
						faceHasVertexTexCoords = true;
						
						int vnI = atoi(groupTokens[2].c_str());
						if( vnI < 0 )
							vnI = vertexNormals.size() + vnI + 1;
						
						vnI--;
						vn.push_back( vnI ); 
						objHasVertexNormals = true; 
						faceHasVertexNormals = true;					
					} else if(groupTokens.size() != 1) {
					}
				}    
				
				//now the local variables have been filled up; push them onto our global 
				//variables appropriately.
				
				for(long unsigned int i = 1; i < v.size()-1; i++) {
					Face *f = new Face();
					f->setMaterial( materialForFace );
					f->setTextureHandle( textureForFace );
					f->setSmooth( smoothForFace );
										
					if( faceHasVertexNormals ) {
						f->setPNormal( Vector( vertexNormals.at( vn[0]*3+0 ), vertexNormals.at( vn[0]*3+1 ), vertexNormals.at( vn[0]*3+2 ) ) ); 
					} else {
						Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
						Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
						Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
						Vector normal = cross( v2-v1, v3-v1 );
						normal.normalize();
						
						f->setPNormal( normal );
					}
					if( faceHasVertexTexCoords ) {
						f->setPTexCoord( Point( vertexTexCoords.at( vt[0]*2+0 ), vertexTexCoords.at( vt[0]*2+1 ), 0.0f ) );
					}
					f->setP( Point( vertices.at( v[0]*3+0 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) ) );
					
					if( faceHasVertexNormals ) {
						f->setQNormal( Vector( vertexNormals.at( vn[i]*3+0 ), vertexNormals.at( vn[i]*3+1 ), vertexNormals.at( vn[i]*3+2 ) ) );
					} else {
						Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
						Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
						Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
						Vector normal = cross( v3-v2, v1-v2 );
						normal.normalize();
						
						f->setQNormal( normal );
					}
					if( faceHasVertexTexCoords ) {
						f->setQTexCoord( Point( vertexTexCoords.at( vt[i]*2+0 ), vertexTexCoords.at( vt[i]*2+1 ), 0.0f ) );
					}
					f->setQ( Point( vertices.at( v[i]*3+0 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) ) );
					
					if( faceHasVertexNormals ) {
						f->setRNormal( Vector( vertexNormals.at( vn[i+1]*3+0 ), vertexNormals.at( vn[i+1]*3+1 ), vertexNormals.at( vn[i+1]*3+2 ) ) );
					} else {
						Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
						Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
						Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
						Vector normal = cross( v1-v3, v2-v3 );
						normal.normalize();

						f->setRNormal( normal );
					}
					if( faceHasVertexTexCoords ) {
						f->setRTexCoord( Point( vertexTexCoords.at( vt[i+1]*2+0 ), vertexTexCoords.at( vt[i+1]*2+1 ), 0.0f ) );
					}
					f->setR( Point( vertices.at( v[i+1]*3+0 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) ) );	
					
					faces->push_back( f );
				} 
			}
		}
		in.close();
		
		return faces;
	}

	vector< Point* >* Object::getVertices() {
		vector< Point* > *resultantVertices = new vector< Point* >();

		for( unsigned int i = 0; i < vertices.size(); i+=3 ) {
			resultantVertices->push_back( new Point( vertices[i+0], vertices[i+1], vertices[i+2] ) );
		}

		return resultantVertices;
	}

	void Object::init() {
		objHasVertexTexCoords = false;
		objHasVertexNormals = false;

		_location = new Point(0.0, 0.0, 0.0);
		
		_materials = new map< string, Material* >();
		_textureHandles = new map< string, GLuint >();
	}

	/*
	 * Read in a WaveFront *.obj File
	 */
	bool Object::loadOBJFile( bool INFO, bool ERRORS ) {
		bool result = true;
			
		if (INFO ) cout << "[.obj]: -=-=-=-=-=-=-=- BEGIN " << _objFile << " Info -=-=-=-=-=-=-=- " << endl;
		
		time_t start, end;
		time(&start);
		
		ifstream in( _objFile.c_str() );
		if( !in.is_open() ) {
			if (ERRORS) cout << "[.obj]: [ERROR]: Could not open \"" << _objFile << "\"" << endl;
			if ( INFO ) cout << "[.obj]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
			return false;
		}

		int numFaces = 0, numTriangles = 0;
		float minX = 999999, maxX = -999999, minY = 999999, maxY = -999999, minZ = 999999, maxZ = -999999;
		string line;

		Material *solidWhiteMaterial = new Material( GOL_MATERIAL_WHITE );

	    _objectDisplayList = glGenLists(1);

	    glNewList(_objectDisplayList, GL_COMPILE); {
			
			int progressCounter = 0;

			while( getline( in, line ) ) {
				line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
				
				vector< string > tokens = tokenizeString( line, " \t" );
				if( tokens.size() < 1 ) continue;
				
				//the line should have a single character that lets us know if it's a...
				if( !tokens[0].compare( "#" ) || tokens[0].find_first_of("#") == 0 ) {								// comment ignore
				} else if( !tokens[0].compare( "o" ) ) {						// object name ignore
				} else if( !tokens[0].compare( "g" ) ) {						// polygon group name ignore
				} else if( !tokens[0].compare( "mtllib" ) ) {					// material library
					_mtlFile = tokens[1];
					loadMTLFile( INFO, ERRORS );
				} else if( !tokens[0].compare( "usemtl" ) ) {					// use material library
					map< string, Material* >::iterator materialIter = _materials->find( tokens[1] );
					if( materialIter != _materials->end() ) {
						setCurrentMaterial( materialIter->second );
					} else {

					}
					
					map< string, GLuint >::iterator textureIter = _textureHandles->find( tokens[1] );
					if( textureIter != _textureHandles->end() ) {
						glEnable( GL_TEXTURE_2D );
						glBindTexture( GL_TEXTURE_2D, textureIter->second );
					} else {
						glDisable( GL_TEXTURE_2D );
					}
				} else if( !tokens[0].compare( "s" ) ) {						// smooth shading
					if( !tokens[1].compare( "off" ) ) {
						glShadeModel( GL_FLAT );
					} else {
						glShadeModel( GL_SMOOTH );
					}
				} else if( !tokens[0].compare( "v" ) ) {						//vertex
					float x = atof( tokens[1].c_str() ),
						  y = atof( tokens[2].c_str() ),
						  z = atof( tokens[3].c_str() );
					
					if( x < minX ) minX = x;
					if( x > maxX ) maxX = x;
					if( y < minY ) minY = y;
					if( y > maxY ) maxY = y;
					if( z < minZ ) minZ = z;
					if( z > maxZ ) maxZ = z;
					
					vertices.push_back( x );
					vertices.push_back( y );
					vertices.push_back( z );
				} else if( !tokens.at(0).compare( "vn" ) ) {                    //vertex normal
					vertexNormals.push_back( atof( tokens[1].c_str() ) );
					vertexNormals.push_back( atof( tokens[2].c_str() ) );
					vertexNormals.push_back( atof( tokens[3].c_str() ) );
				} else if( !tokens.at(0).compare( "vt" ) ) {                    //vertex tex coord
					vertexTexCoords.push_back(atof(tokens[1].c_str()));
					vertexTexCoords.push_back(atof(tokens[2].c_str()));
				} else if( !tokens.at(0).compare( "f" ) ) {                     //face!
					
					//now, faces can be either quads or triangles (or maybe more?)
					//split the string on spaces to get the number of verts+attrs.
					vector<string> faceTokens = tokenizeString(line, " ");
					
					//some local variables to hold the vertex+attribute indices we read in.
					//we do it this way because we'll have to split quads into triangles ourselves.
					vector<int> v, vn, vt;
					
					bool faceHasVertexTexCoords = false, faceHasVertexNormals = false;
					
					for(long unsigned int i = 1; i < faceTokens.size(); i++) {
						//need to use both the tokens and number of slashes to determine what info is there.
						vector<string> groupTokens = tokenizeString(faceTokens[i], "/");
						int numSlashes = 0;
						for(long unsigned int j = 0; j < faceTokens[i].length(); j++) { if(faceTokens[i][j] == '/') numSlashes++; }
						
						int vert = atoi(groupTokens[0].c_str());
						if( vert < 0 )
							vert = (vertices.size() / 3) + vert + 1;
						
						//regardless, we always get a vertex index.
						v.push_back( vert - 1 );
						
						//based on combination of number of tokens and slashes, we can determine what we have.
						if(groupTokens.size() == 2 && numSlashes == 1) {
							int vtI = atoi(groupTokens[1].c_str());	
							if( vtI < 0 )
								vtI = (vertexTexCoords.size() / 2) + vtI + 1;
							
							vtI--;
							vt.push_back( vtI ); 
							objHasVertexTexCoords = true; 
							faceHasVertexTexCoords = true;
						} else if(groupTokens.size() == 2 && numSlashes == 2) {
							int vnI = atoi(groupTokens[1].c_str());
							if( vnI < 0 )
								vnI = vertexNormals.size() + vnI + 1;
							
							vnI--;
							vn.push_back( vnI ); 
							objHasVertexNormals = true; 
							faceHasVertexNormals = true;
						} else if(groupTokens.size() == 3) {
							int vtI = atoi(groupTokens[1].c_str());
							if( vtI < 0 )
								vtI = vertexTexCoords.size() + vtI + 1;
							
							vtI--;
							vt.push_back( vtI ); 
							objHasVertexTexCoords = true; 
							faceHasVertexTexCoords = true;
							
							int vnI = atoi(groupTokens[2].c_str());
							if( vnI < 0 )
								vnI = vertexNormals.size() + vnI + 1;

							vnI--;
							vn.push_back( vnI ); 
							objHasVertexNormals = true; 
							faceHasVertexNormals = true;					
						} else if(groupTokens.size() != 1) {
							if (ERRORS) fprintf(stderr, "[.obj]: [ERROR]: Malformed OBJ file, %s.\n", _objFile.c_str());
							return false;
						}
					}    
					
					//now the local variables have been filled up; push them onto our global 
					//variables appropriately.
					
					glBegin(GL_TRIANGLES); {
						for(long unsigned int i = 1; i < v.size()-1; i++) {
							
							if( faceHasVertexNormals ) {
								glNormal3f( vertexNormals.at( vn[0]*3 ), vertexNormals.at( vn[0]*3+1 ), vertexNormals.at( vn[0]*3+2) );
							} else {
								Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
								Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
								Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
								Vector normal = cross( v2-v1, v3-v1 );
								normal.normalize();
								glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							}
							if( faceHasVertexTexCoords )
								glTexCoord2f( vertexTexCoords.at( vt[0]*2 ), vertexTexCoords.at( vt[0]*2+1 ) );
							
							glVertex3f( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							
							if( faceHasVertexNormals ) {
								glNormal3f( vertexNormals.at( vn[i]*3 ), vertexNormals.at( vn[i]*3+1 ), vertexNormals.at( vn[i]*3+2 ) );
							} else {
								Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
								Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
								Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
								Vector normal = cross( v3-v2, v1-v2 );
								normal.normalize();
								glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							}
							if( faceHasVertexTexCoords )
								glTexCoord2f( vertexTexCoords.at( vt[i]*2 ), vertexTexCoords.at( vt[i]*2+1 ) );
							glVertex3f( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							
							if( faceHasVertexNormals ) {
								glNormal3f( vertexNormals.at( vn[i+1]*3 ), vertexNormals.at( vn[i+1]*3+1 ), vertexNormals.at( vn[i+1]*3+2) );
							} else {
								Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
								Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
								Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
								Vector normal = cross( v1-v3, v2-v3 );
								normal.normalize();
								glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							}
							if( faceHasVertexTexCoords )
								glTexCoord2f( vertexTexCoords.at( vt[i+1]*2 ), vertexTexCoords.at( vt[i+1]*2+1 ) );
							glVertex3f( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );				
							
							numTriangles++;
						} 
					}; glEnd();
					
					numFaces++;
				} else {
					if (INFO) cout << "[.obj]: ignoring line: " << line << endl;
				}
				
				if (INFO) {
					progressCounter++;
					if( progressCounter % 5000 == 0 ) {					
						printf("\33[2K\r");
						switch( progressCounter ) {
							case 5000:	printf("[.obj]: reading in %s...\\", _objFile.c_str());	break;
							case 10000:	printf("[.obj]: reading in %s...|", _objFile.c_str());	break;
							case 15000:	printf("[.obj]: reading in %s.../", _objFile.c_str());	break;
							case 20000:	printf("[.obj]: reading in %s...-", _objFile.c_str());	break;
						}
						fflush(stdout);
					}
					if( progressCounter == 20000 )
						progressCounter = 0;	   
				}
			}
			in.close();
			setCurrentMaterial( solidWhiteMaterial );
			glDisable( GL_TEXTURE_2D );
		}; glEndList();
		
		time(&end);
		double seconds = difftime( end, start );
		
		if (INFO) {
			printf("\33[2K\r");
			printf("[.obj]: reading in %s...done!  (Time: %.1fs)\n", _objFile.c_str(), seconds);
			cout << "[.obj]: Vertices:  \t" << vertices.size()/3
					<< "\tNormals:   \t" << vertexNormals.size()/3
					<< "\tTex Coords:\t" << vertexTexCoords.size()/2 << endl
				 << "[.obj]: Faces:     \t" << numFaces
					<< "\tTriangles: \t" << numTriangles << endl
				 << "[.obj]: Dimensions:\t(" << (maxX - minX) << ", " << (maxY - minY) << ", " << (maxZ - minZ) << ")" << endl;
			cout << "[.obj]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
		}
		
		return result;
	}

	bool Object::loadMTLFile( bool INFO, bool ERRORS ) {
		bool result = true;
		
		if (INFO) cout << "[.mtl]: -*-*-*-*-*-*-*- BEGIN " << _mtlFile << " Info -*-*-*-*-*-*-*- " << endl;
		
		string line;
		string path = _objFile.substr( 0, _objFile.find_last_of("/")+1 );
		
		ifstream in;
		in.open( _mtlFile.c_str() );
		if( !in.is_open() ) {
			string folderMtlFile = path + _mtlFile;
			in.open( folderMtlFile.c_str() );
			if( !in.is_open() ) {
				if (ERRORS) cerr << "[.mtl]: [ERROR]: could not open material file: " << _mtlFile << endl;
				if ( INFO ) cout << "[.mtl]: -*-*-*-*-*-*-*-  END " << _mtlFile << " Info  -*-*-*-*-*-*-*- " << endl;
				return false;
			}
		}
		
		Material *currMaterial;
		string materialName;
		
		unsigned char *textureData = NULL;
		unsigned char *maskData = NULL;
		unsigned char *fullData;
		int texWidth, texHeight, textureChannels = 1, maskChannels = 1;
		GLuint textureHandle = 0;
		
		map< string, GLuint > imageHandles;
		
		int numMaterials = 0;
		
		while( getline( in, line ) ) {
			line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
			
			vector< string > tokens = tokenizeString( line, " /" );
			if( tokens.size() < 1 ) continue;
			
			//the line should have a single character that lets us know if it's a...
			if( !tokens[0].compare( "#" ) ) {							// comment
			} else if( !tokens[0].compare( "newmtl" ) ) {				//new material
				currMaterial = new Material();
				materialName = tokens[1];
				_materials->insert( pair<string, Material*>( materialName, currMaterial ) );

				textureHandle = 0;
				textureData = NULL;
				maskData = NULL;
				textureChannels = 1;
				maskChannels = 1;
				
				numMaterials++;
			} else if( !tokens[0].compare( "Ka" ) ) {					// ambient component
				GLfloat ambient[4];
				ambient[0] = atof( tokens[1].c_str() );
				ambient[1] = atof( tokens[2].c_str() );
				ambient[2] = atof( tokens[3].c_str() );
				ambient[3] = 1;
				currMaterial->setAmbient( ambient );
			} else if( !tokens[0].compare( "Kd" ) ) {					// diffuse component
				GLfloat diffuse[4];
				diffuse[0] = atof( tokens[1].c_str() );
				diffuse[1] = atof( tokens[2].c_str() );
				diffuse[2] = atof( tokens[3].c_str() );
				diffuse[3] = 1;			
				currMaterial->setDiffuse( diffuse );
			} else if( !tokens[0].compare( "Ks" ) ) {					// specular component
				GLfloat specular[4];
				specular[0] = atof( tokens[1].c_str() );
				specular[1] = atof( tokens[2].c_str() );
				specular[2] = atof( tokens[3].c_str() );
				specular[3] = 1;						
				currMaterial->setSpecular( specular );
			} else if( !tokens[0].compare( "Ke" ) ) {					// emissive component
				GLfloat emissive[4];
				emissive[0] = atof( tokens[1].c_str() );
				emissive[1] = atof( tokens[2].c_str() );
				emissive[2] = atof( tokens[3].c_str() );
				currMaterial->setEmissive( emissive );
			} else if( !tokens[0].compare( "Ns" ) ) {					// shininess component
				currMaterial->setShininess( atof( tokens[1].c_str() ) );
			} else if( !tokens[0].compare( "Tr" ) 
					  || !tokens[0].compare( "d" ) ) {					// transparency component - Tr or d can be used depending on the format
				currMaterial->getAmbient()[3] = atof( tokens[1].c_str() );
				currMaterial->getDiffuse()[3] = atof( tokens[1].c_str() );
				currMaterial->getSpecular()[3] = atof( tokens[1].c_str() );
			} else if( !tokens[0].compare( "illum" ) ) {				// illumination type component
				currMaterial->setIllumination( atoi( tokens[1].c_str() ) );
			} else if( !tokens[0].compare( "map_Kd" ) ) {				// diffuse color texture map
				if( imageHandles.find( tokens[1] ) != imageHandles.end() ) {
					_textureHandles->insert( pair< string, GLuint >( materialName, imageHandles.find( tokens[1] )->second ) );
				} else {
					if( tokens[1].find( ".bmp" ) != string::npos || tokens[1].find( ".BMP" ) != string::npos ) {
						bool success = false;
						textureData = loadBMP( (char*)tokens[1].c_str(), texWidth, texHeight, textureChannels, success, ERRORS, path );
						if( !success ) {
							textureData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							if( !textureData ) {
								string folderName = path + tokens[1];
								textureData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							}
						} else {
							
						}
					} else if( tokens[1].find( ".ppm" ) != string::npos || tokens[1].find( ".PPM" ) != string::npos ) {
						bool success = false;
						textureData = loadPPM( (char*)tokens[1].c_str(), texWidth, texHeight, textureChannels, success, ERRORS, path );
						if( !success ) {
							textureData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							if( !textureData ) {
								string folderName = path + tokens[1];
								textureData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							}
						} else {
							
						}
					} /*else if( tokens[1].find( ".tga" ) != string::npos || tokens[1].find( ".TGA" ) != string::npos ) {
						bool success = false;
						textureData = loadTGA( (char*)tokens[1].c_str(), texWidth, texHeight, textureChannels, success, ERRORS, path );
						if( !success ) {
							textureData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							if( !textureData ) {
								string folderName = path + tokens[1];
								textureData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
							}
						}
					}*/ else {
						textureData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
						if( !textureData ) {
							string folderName = path + tokens[1];
							textureData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO );
						}
					}

					if( !textureData ) {
						cout << "[.mtl]: [ERROR]: File Not Found: " << tokens[1] << endl;
					} else {
						if (INFO) cout << "[.mtl]: TextureMap:\t" << tokens[1] << "\tSize: " << texWidth << "x" 
										<< texHeight << "\tColors: " << textureChannels << endl;
						
						if( maskData == NULL ) {
							if( textureHandle == 0 )
								glGenTextures( 1, &textureHandle );
							
							glBindTexture( GL_TEXTURE_2D, textureHandle );
							
							glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							
							GLenum colorSpace = GL_RGB;
							if( textureChannels == 4 )
								colorSpace = GL_RGBA;
							gluBuild2DMipmaps( GL_TEXTURE_2D, colorSpace, texWidth, texHeight, colorSpace, GL_UNSIGNED_BYTE, textureData );		
				
							_textureHandles->insert( pair<string, GLuint>( materialName, textureHandle ) );
							imageHandles.insert( pair<string, GLuint>( tokens[1], textureHandle ) );
						} else {					
							fullData = createTransparentTexture( textureData, maskData, texWidth, texHeight, textureChannels, maskChannels );
							
							if( textureHandle == 0 )
								glGenTextures( 1, &textureHandle );
							
							glBindTexture( GL_TEXTURE_2D, textureHandle );
							
							glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							
							gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, fullData );

							delete fullData;
				
							_textureHandles->insert( pair<string, GLuint>( materialName, textureHandle ) );
						}
					}
				}
			} else if( !tokens[0].compare( "map_d" ) ) {				// alpha texture map
				if( imageHandles.find( tokens[1] ) != imageHandles.end() ) {
					_textureHandles->insert( pair< string, GLuint >( materialName, imageHandles.find( tokens[1] )->second ) );
				} else {
					if( tokens[1].find( ".bmp" ) != string::npos || tokens[1].find( ".BMP" ) != string::npos ) {
						bool success = false;
						maskData = loadBMP( (char*)tokens[1].c_str(), texWidth, texHeight, maskChannels, success, ERRORS, path );
						if( !success ) {
							maskData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
							if( !maskData ) {
								string folderName = path + tokens[1];
								maskData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
							}
						} else {
							
						}
					} else if( tokens[1].find( ".ppm" ) != string::npos || tokens[1].find( ".PPM" ) != string::npos ) {
						bool success = false;
						maskData = loadPPM( (char*)tokens[1].c_str(), texWidth, texHeight, maskChannels, success, ERRORS, path );
						if( !success ) {
							maskData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
							if( !maskData ) {
								string folderName = path + tokens[1];
								maskData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
							}
						} else {
							
						}
					} else {
						maskData = SOIL_load_image( tokens[1].c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
						if( !maskData ) {
							string folderName = path + tokens[1];
							maskData = SOIL_load_image( folderName.c_str(), &texWidth, &texHeight, &maskChannels, SOIL_LOAD_AUTO );
						}
					}
					
					if( !maskData ) {
						cout << "[.mtl]: [ERROR]: File Not Found: " << tokens[1] << endl;
					} else {
						if (INFO) cout << "[.mtl]: AlphaMap:  \t" << tokens[1] << "\tSize: " << texWidth << "x" 
										<< texHeight << "\tColors: " << maskChannels << endl;
						
						if( textureData != NULL ) {
							fullData = createTransparentTexture( textureData, maskData, texWidth, texHeight, textureChannels, maskChannels );
							
							if (INFO) cout << "[.mtl]: " << tokens[1] << " alpha texture map of size " << texWidth << "x" 
											<< texHeight << " with " << textureChannels << " colors read in" << endl;
							
							if( textureHandle == 0 )
								glGenTextures( 1, &textureHandle );
							
							glBindTexture( GL_TEXTURE_2D, textureHandle );
							
							glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
							
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							
							gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, fullData );
							
							delete fullData;
						}
					}
				}
			} else if( !tokens[0].compare( "map_Ka" ) ) {				// ambient color texture map
				
			} else if( !tokens[0].compare( "map_Ks" ) ) {				// specular color texture map
				
			} else if( !tokens[0].compare( "map_Ns" ) ) {				// specular highlight map (shininess map)
				
			} else if( !tokens[0].compare( "Ni" ) ) {					// optical density / index of refraction
				
			} else if( !tokens[0].compare( "Tf" ) ) {					// transmission filter
				
			} else if( !tokens[0].compare( "bump" ) 
					  || !tokens[0].compare( "map_bump" ) ) {			// bump map
			  
			} else {
				if (INFO) cout << "[.mtl]: ignoring line: " << line << endl;
			}
		}
		
		in.close();
		
		if ( INFO ) {
			cout << "[.mtl]: Materials:\t" << numMaterials << endl;
			cout << "[.mtl]: -*-*-*-*-*-*-*-  END " << _mtlFile << " Info  -*-*-*-*-*-*-*- " << endl;
		}
		
		return result;
	}
	
	bool Object::loadOFFFile( bool INFO, bool ERRORS ) {
		bool result = true;

		if (INFO ) cout << "[.off]: -=-=-=-=-=-=-=- BEGIN " << _objFile << " Info -=-=-=-=-=-=-=- " << endl;
		
		time_t start, end;
		time(&start);
		
		ifstream in( _objFile.c_str() );
		if( !in.is_open() ) {
			if (ERRORS) cout << "[.off]: [ERROR]: Could not open \"" << _objFile << "\"" << endl;
			if ( INFO ) cout << "[.off]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
			return false;
		}

		int numVertices = 0, numFaces = 0, numTriangles = 0;
		float minX = 999999, maxX = -999999, minY = 999999, maxY = -999999, minZ = 999999, maxZ = -999999;
		string line;

		enum OFF_FILE_STATE { HEADER, VERTICES, FACES };

		OFF_FILE_STATE fileState = HEADER;
		Material *tMat = new Material( GOL_MATERIAL_BLACK );

	    _objectDisplayList = glGenLists(1);

	    glNewList(_objectDisplayList, GL_COMPILE); {
			
			int progressCounter = 0;

			while( getline( in, line ) ) {
				line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
				
				vector< string > tokens = tokenizeString( line, " \t" );
				if( tokens.size() < 1 ) continue;
				
				//the line should have a single character that lets us know if it's a...
				if( !tokens[0].compare( "#" ) || tokens[0].find_first_of("#") == 0 ) {								// comment ignore
				} else if( fileState == HEADER ) {
					if( !tokens[0].compare( "OFF" ) ) {					// denotes OFF File type
					} else {
						if( tokens.size() != 3 ) {
							result = false;
							if (ERRORS) cout << "[.off]: [ERROR]: Malformed OFF file.  # vertices, faces, edges not properly specified" << endl;
							break;
						}
						/* read in number of expected vertices, faces, and edges */
						numVertices = atoi( tokens[0].c_str() );
						numFaces = atoi( tokens[1].c_str() );
						/* ignore tokens[2] - number of edges -- unnecessary information */
						// numEdges = atoi( tokens[2].c_str() );

						/* end of OFF Header reached */
						fileState = VERTICES;
					}
				} else if( fileState == VERTICES ) {
					/* read in x y z vertex location */
					float x = atof( tokens[0].c_str() ),
					      y = atof( tokens[1].c_str() ),
					      z = atof( tokens[2].c_str() );
					
					if( x < minX ) minX = x;
					if( x > maxX ) maxX = x;
					if( y < minY ) minY = y;
					if( y > maxY ) maxY = y;
					if( z < minZ ) minZ = z;
					if( z > maxZ ) maxZ = z;
					
					vertices.push_back( x );
					vertices.push_back( y );
					vertices.push_back( z );
					
					/* check if RGB(A) color information is associated with vertex */
					if( tokens.size() == 6 || tokens.size() == 7 ) {
						float r = atof( tokens[3].c_str() ),
							  g = atof( tokens[4].c_str() ),
							  b = atof( tokens[5].c_str() ),
							  a = 1;
						if( tokens.size() == 7 )
							a = atof( tokens[6].c_str() );

						vertexColors.push_back( r );
						vertexColors.push_back( g );
						vertexColors.push_back( b );
						vertexColors.push_back( a );
					}

					numVertices--;
					/* if all vertices have been read in, move on to faces */
					if( numVertices == 0 )
						fileState = FACES;
				} else if( fileState == FACES ) {
					unsigned int numberOfVerticesInFace = atoi( tokens[0].c_str() );
					
					//some local variables to hold the vertex+attribute indices we read in.
					//we do it this way because we'll have to split quads into triangles ourselves.
					vector<unsigned int> v;
					GLfloat color[4] = {-1,-1,-1,1};
						
					/* read in each vertex index of the face */
					for(unsigned int i = 1; i <= numberOfVerticesInFace; i++) {
						int vert = atoi( tokens[i].c_str() );
						if( vert < 0 )
							vert = (vertices.size() / 3) + vert + 1;
						
						//regardless, we always get a vertex index.
						v.push_back( vert );
					}

					/* check if RGB(A) color information is associated with face */
					if( tokens.size() == numberOfVerticesInFace + 4 || tokens.size() == numberOfVerticesInFace + 5 ) {
						color[0] = atof( tokens[numberOfVerticesInFace + 1].c_str() );
						color[1] = atof( tokens[numberOfVerticesInFace + 2].c_str() );
						color[2] = atof( tokens[numberOfVerticesInFace + 3].c_str() );
						color[3] = 1;

						if( tokens.size() == numberOfVerticesInFace + 5 )
							color[3] = atof( tokens[numberOfVerticesInFace + 4].c_str() );
					}
					
					//now the local variables have been filled up; push them onto our global 
					//variables appropriately.
					
					glBegin(GL_TRIANGLES); {
						for(unsigned int i = 1; i < v.size()-1; i++) {
							
							/* vertex 1 */
							/* calculate normal */
							Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							Vector normal = cross( v2-v1, v3-v1 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( color[0] != -1 ) {
								tMat->setAmbient( color );
								tMat->setDiffuse( color );
								setCurrentMaterial( tMat );
							} else if( vertexColors.size() >= v[0]*4+3 ) {
								glColor4f( vertexColors.at( v[0]*4 ), vertexColors.at( v[0]*4+1 ), vertexColors.at( v[0]*4+2 ), vertexColors.at( v[0]*4+3 ) );
							}

							glVertex3f( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							
							/* vertex 2 */
							/* calculate normal */
							v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							normal = cross( v3-v2, v1-v2 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( vertexColors.size() >= v[i]*4+3 ) {
								glColor4f( vertexColors.at( v[i]*4 ), vertexColors.at( v[i]*4+1 ), vertexColors.at( v[i]*4+2 ), vertexColors.at( v[i]*4+3 ) );
							}
							
							glVertex3f( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							
							/* vertex 3 */
							/* calculate normal */
							v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							normal = cross( v1-v3, v2-v3 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( vertexColors.size() >= v[i+1]*4+3 ) {
								glColor4f( vertexColors.at( v[i+1]*4 ), vertexColors.at( v[i+1]*4+1 ), vertexColors.at( v[i+1]*4+2 ), vertexColors.at( v[i+1]*4+3 ) );
							}
							
							glVertex3f( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );				
							
							numTriangles++;
						} 
					}; glEnd();
								
				} else {
					if (INFO) cout << "[.off]: unknown file state: " << fileState << endl;
				}
				
				if (INFO) {
					progressCounter++;
					if( progressCounter % 5000 == 0 ) {					
						printf("\33[2K\r");
						switch( progressCounter ) {
							case 5000:	printf("[.off]: reading in %s...\\", _objFile.c_str());	break;
							case 10000:	printf("[.off]: reading in %s...|", _objFile.c_str());	break;
							case 15000:	printf("[.off]: reading in %s.../", _objFile.c_str());	break;
							case 20000:	printf("[.off]: reading in %s...-", _objFile.c_str());	break;
						}
						fflush(stdout);
					}
					if( progressCounter == 20000 )
						progressCounter = 0;	   
				}
			}
			in.close();
			
		}; glEndList();
		
		time(&end);
		double seconds = difftime( end, start );
		
		if (INFO) {
			printf("\33[2K\r");
			printf("[.obj]: reading in %s...done!  (Time: %.1fs)\n", _objFile.c_str(), seconds);
			cout << "[.off]: Vertices:  \t" << vertices.size()/3
					<< "\tNormals:   \t" << vertexNormals.size()/3
					<< "\tTex Coords:\t" << vertexTexCoords.size()/2 << endl
				 << "[.off]: Faces:     \t" << numFaces
					<< "\tTriangles: \t" << numTriangles << endl
				 << "[.off]: Dimensions:\t(" << (maxX - minX) << ", " << (maxY - minY) << ", " << (maxZ - minZ) << ")" << endl;
			cout << "[.off]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
		}

		return result;
	}

	bool Object::loadPLYFile( bool INFO, bool ERRORS ) {
		bool result = true;

		if (INFO ) cout << "[.ply]: -=-=-=-=-=-=-=- BEGIN " << _objFile << " Info -=-=-=-=-=-=-=- " << endl;
		
		time_t start, end;
		time(&start);
		
		ifstream in( _objFile.c_str() );
		if( !in.is_open() ) {
			if (ERRORS) cout << "[.ply]: [ERROR]: Could not open \"" << _objFile << "\"" << endl;
			if ( INFO ) cout << "[.ply]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
			return false;
		}

		int numVertices = 0, numFaces = 0, numTriangles = 0, numMaterials = 0;
		float minX = 999999, maxX = -999999, minY = 999999, maxY = -999999, minZ = 999999, maxZ = -999999;
		string line;

		enum PLY_FILE_STATE { HEADER, VERTICES, FACES, MATERIALS };
		enum PLY_ELEMENT_TYPE { NONE, VERTEX, FACE, MATERIAL };

		PLY_FILE_STATE fileState = HEADER;
		PLY_ELEMENT_TYPE elemType = NONE;
		Material *tMat = new Material( GOL_MATERIAL_BLACK );

	    _objectDisplayList = glGenLists(1);

	    glNewList(_objectDisplayList, GL_COMPILE); {
			
			int progressCounter = 0;

			while( getline( in, line ) ) {
				line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
				
				vector< string > tokens = tokenizeString( line, " \t" );
				
				if( tokens.size() < 1 ) continue;
				
				//the line should have a single character that lets us know if it's a...
				if( !tokens[0].compare( "comment" ) ) {								// comment ignore
				} else if( fileState == HEADER ) {
					if( !tokens[0].compare( "ply" ) ) {					// denotes ply File type
					} else if( !tokens[0].compare( "format" ) ) {
					} else if( !tokens[0].compare( "element" ) ) {		// an element (vertex, face, material)
						if( !tokens[1].compare( "vertex" ) ) {
							numVertices = atoi( tokens[2].c_str() );
							elemType = VERTEX;
						} else if( !tokens[1].compare( "face" ) ) {
							numFaces = atoi( tokens[2].c_str() );
							elemType = FACE;
						} else if( !tokens[1].compare( "edge" ) ) {
						
						} else if( !tokens[1].compare( "material" ) ) {
							numMaterials = atoi( tokens[2].c_str() );
							elemType = MATERIAL;
						} else {

						}
					} else if( !tokens[0].compare( "property" ) ) {
						if( elemType == VERTEX ) {

						} else if( elemType == FACE ) {

						} else if( elemType == MATERIAL ) {

						}
					} else if( !tokens[0].compare( "end_header" ) ) {	// end of the header section
						fileState = VERTICES;
					} 
				} else if( fileState == VERTICES ) {
					/* read in x y z vertex location */
					float x = atof( tokens[0].c_str() ),
					      y = atof( tokens[1].c_str() ),
					      z = atof( tokens[2].c_str() );
					
					if( x < minX ) minX = x;
					if( x > maxX ) maxX = x;
					if( y < minY ) minY = y;
					if( y > maxY ) maxY = y;
					if( z < minZ ) minZ = z;
					if( z > maxZ ) maxZ = z;
					
					vertices.push_back( x );
					vertices.push_back( y );
					vertices.push_back( z );
					
					/* check if RGB(A) color information is associated with vertex */
					if( tokens.size() == 6 || tokens.size() == 7 ) {
						float r = atof( tokens[3].c_str() )/255.0,
							  g = atof( tokens[4].c_str() )/255.0,
							  b = atof( tokens[5].c_str() )/255.0,
							  a = 1;
						if( tokens.size() == 7 )
							a = atof( tokens[6].c_str() );

						vertexColors.push_back( r );
						vertexColors.push_back( g );
						vertexColors.push_back( b );
						vertexColors.push_back( a );
					}

					numVertices--;
					/* if all vertices have been read in, move on to faces */
					if( numVertices == 0 )
						fileState = FACES;
				} else if( fileState == FACES ) {
					unsigned int numberOfVerticesInFace = atoi( tokens[0].c_str() );
					
					//some local variables to hold the vertex+attribute indices we read in.
					//we do it this way because we'll have to split quads into triangles ourselves.
					vector<unsigned int> v;
					GLfloat color[4] = {-1,-1,-1,1};
						
					/* read in each vertex index of the face */
					for(unsigned int i = 1; i <= numberOfVerticesInFace; i++) {
						int vert = atoi( tokens[i].c_str() );
						if( vert < 0 )
							vert = (vertices.size() / 3) + vert + 1;
						
						//regardless, we always get a vertex index.
						v.push_back( vert );
					}

					/* check if RGB(A) color information is associated with face */
					if( tokens.size() == numberOfVerticesInFace + 4 || tokens.size() == numberOfVerticesInFace + 5 ) {
						color[0] = atof( tokens[numberOfVerticesInFace + 1].c_str() );
						color[1] = atof( tokens[numberOfVerticesInFace + 2].c_str() );
						color[2] = atof( tokens[numberOfVerticesInFace + 3].c_str() );
						color[3] = 1;

						if( tokens.size() == numberOfVerticesInFace + 5 )
							color[3] = atof( tokens[numberOfVerticesInFace + 4].c_str() );
					}
					
					//now the local variables have been filled up; push them onto our global 
					//variables appropriately.
					
					glBegin(GL_TRIANGLES); {
						for(unsigned int i = 1; i < v.size()-1; i++) {
							
							/* vertex 1 */
							/* calculate normal */
							Point v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							Point v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							Point v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							Vector normal = cross( v2-v1, v3-v1 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( color[0] != -1 ) {
								tMat->setAmbient( color );
								tMat->setDiffuse( color );
								setCurrentMaterial( tMat );
							} else if( vertexColors.size() >= v[0]*4+3 ) {
								color[0] = vertexColors.at( v[0]*4 );
								color[1] = vertexColors.at( v[0]*4+1 );
								color[2] = vertexColors.at( v[0]*4+2 );
								color[3] = vertexColors.at( v[0]*4+3 );
								tMat->setAmbient( color );
								tMat->setDiffuse( color );
								setCurrentMaterial( tMat );
								//glColor4f( vertexColors.at( v[0]*4 ), vertexColors.at( v[0]*4+1 ), vertexColors.at( v[0]*4+2 ), vertexColors.at( v[0]*4+3 ) );
							}

							glVertex3f( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							
							/* vertex 2 */
							/* calculate normal */
							v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							normal = cross( v3-v2, v1-v2 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( vertexColors.size() >= v[i]*4+3 ) {
								color[0] = vertexColors.at( v[i]*4 );
								color[1] = vertexColors.at( v[i]*4+1 );
								color[2] = vertexColors.at( v[i]*4+2 );
								color[3] = vertexColors.at( v[i]*4+3 );
								tMat->setAmbient( color );
								tMat->setDiffuse( color );
								setCurrentMaterial( tMat );
								//glColor4f( vertexColors.at( v[i]*4 ), vertexColors.at( v[i]*4+1 ), vertexColors.at( v[i]*4+2 ), vertexColors.at( v[i]*4+3 ) );
							}
							
							glVertex3f( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							
							/* vertex 3 */
							/* calculate normal */
							v1 = Point( vertices.at( v[0]*3 ), vertices.at( v[0]*3+1 ), vertices.at( v[0]*3+2 ) );
							v2 = Point( vertices.at( v[i]*3 ), vertices.at( v[i]*3+1 ), vertices.at( v[i]*3+2 ) );
							v3 = Point( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );
							normal = cross( v1-v3, v2-v3 );
							normal.normalize();
							glNormal3f( normal.getX(), normal.getY(), normal.getZ() );
							
							/* set color if exists */
							if( vertexColors.size() >= v[i+1]*4+3 ) {
								color[0] = vertexColors.at( v[i+1]*4 );
								color[1] = vertexColors.at( v[i+1]*4+1 );
								color[2] = vertexColors.at( v[i+1]*4+2 );
								color[3] = vertexColors.at( v[i+1]*4+3 );
								tMat->setAmbient( color );
								tMat->setDiffuse( color );
								setCurrentMaterial( tMat );
								//glColor4f( vertexColors.at( v[i+1]*4 ), vertexColors.at( v[i+1]*4+1 ), vertexColors.at( v[i+1]*4+2 ), vertexColors.at( v[i+1]*4+3 ) );
							}
							
							glVertex3f( vertices.at( v[i+1]*3 ), vertices.at( v[i+1]*3+1 ), vertices.at( v[i+1]*3+2 ) );				
							
							numTriangles++;
						} 
					}; glEnd();
								
				} else {
					if (INFO) cout << "[.ply]: unknown file state: " << fileState << endl;
				}
				
				if (INFO) {
					progressCounter++;
					if( progressCounter % 5000 == 0 ) {					
						printf("\33[2K\r");
						switch( progressCounter ) {
							case 5000:	printf("[.ply]: reading in %s...\\", _objFile.c_str());	break;
							case 10000:	printf("[.ply]: reading in %s...|", _objFile.c_str());	break;
							case 15000:	printf("[.ply]: reading in %s.../", _objFile.c_str());	break;
							case 20000:	printf("[.ply]: reading in %s...-", _objFile.c_str());	break;
						}
						fflush(stdout);
					}
					if( progressCounter == 20000 )
						progressCounter = 0;	   
				}
			}
			in.close();
			
		}; glEndList();
		
		time(&end);
		double seconds = difftime( end, start );
		
		if (INFO) {
			printf("\33[2K\r");
			printf("[.obj]: reading in %s...done!  (Time: %.1fs)\n", _objFile.c_str(), seconds);
			cout << "[.ply]: Vertices:  \t" << vertices.size()/3
					<< "\tNormals:   \t" << vertexNormals.size()/3
					<< "\tTex Coords:\t" << vertexTexCoords.size()/2 << endl
				 << "[.ply]: Faces:     \t" << numFaces
					<< "\tTriangles: \t" << numTriangles << endl
				 << "[.ply]: Dimensions:\t(" << (maxX - minX) << ", " << (maxY - minY) << ", " << (maxZ - minZ) << ")" << endl;
			cout << "[.ply]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
		}

		return result;
	}

	bool Object::loadSTLFile( bool INFO, bool ERRORS ) {
		bool result = true;

		if (INFO ) cout << "[.stl]: -=-=-=-=-=-=-=- BEGIN " << _objFile << " Info -=-=-=-=-=-=-=- " << endl;
		
		time_t start, end;
		time(&start);
		
		ifstream in( _objFile.c_str() );
		if( !in.is_open() ) {
			if (ERRORS) cout << "[.stl]: [ERROR]: Could not open \"" << _objFile << "\"" << endl;
			if ( INFO ) cout << "[.stl]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
			return false;
		}

		int numVertices = 0, numFaces = 0, numTriangles = 0;
		float minX = 999999, maxX = -999999, minY = 999999, maxY = -999999, minZ = 999999, maxZ = -999999;
		string line;

	    _objectDisplayList = glGenLists(1);

	    glNewList(_objectDisplayList, GL_COMPILE); {
			
			int progressCounter = 0;
			float normalVector[3] = {0,0,0};

			while( getline( in, line ) ) {
				line.erase( line.find_last_not_of( " \n\r\t" ) + 1 );
				
				vector< string > tokens = tokenizeString( line, " \t" );
				
				if( tokens.size() < 1 ) continue;
				
				//the line should have a single character that lets us know if it's a...
				if( !tokens[0].compare( "solid" ) ) {
				} else if( !tokens[0].compare( "facet" ) ) {
					/* read in x y z triangle normal */
					normalVector[0] = atof( tokens[2].c_str() );
					normalVector[1] = atof( tokens[3].c_str() );
					normalVector[2] = atof( tokens[4].c_str() );
				} else if( !tokens[0].compare( "outer" ) ) {
					glBegin( GL_TRIANGLES );
				} else if( !tokens[0].compare( "vertex" ) ) {
					float x = atof( tokens[1].c_str() ),
						  y = atof( tokens[2].c_str() ),
						  z = atof( tokens[3].c_str() );
					
					if( x < minX ) minX = x;
					if( x > maxX ) maxX = x;
					if( y < minY ) minY = y;
					if( y > maxY ) maxY = y;
					if( z < minZ ) minZ = z;
					if( z > maxZ ) maxZ = z;
					
					vertices.push_back( x );
					vertices.push_back( y );
					vertices.push_back( z );

					vertexNormals.push_back( normalVector[0] );
					vertexNormals.push_back( normalVector[1] );
					vertexNormals.push_back( normalVector[2] );

					glNormal3fv( normalVector );
					glVertex3f( x, y, z );

					numVertices++;
					
				} else if( !tokens[0].compare( "endloop" ) ) {
					glEnd();
				} else if( !tokens[0].compare( "endfacet" ) ) {
					numFaces++;
					numTriangles++;
				} else if( !tokens[0].compare( "endsolid" ) ) {
				
				}
				else {
					if (INFO) cout << "[.stl]: unknown line: " << line << endl;
				}
				
				if (INFO) {
					progressCounter++;
					if( progressCounter % 5000 == 0 ) {					
						printf("\33[2K\r");
						switch( progressCounter ) {
							case 5000:	printf("[.stl]: reading in %s...\\", _objFile.c_str());	break;
							case 10000:	printf("[.stl]: reading in %s...|", _objFile.c_str());	break;
							case 15000:	printf("[.stl]: reading in %s.../", _objFile.c_str());	break;
							case 20000:	printf("[.stl]: reading in %s...-", _objFile.c_str());	break;
						}
						fflush(stdout);
					}
					if( progressCounter == 20000 )
						progressCounter = 0;	   
				}
			}
			in.close();
			
		}; glEndList();
		
		time(&end);
		double seconds = difftime( end, start );
		
		if (INFO) {
			printf("\33[2K\r");
			printf("[.obj]: reading in %s...done!  (Time: %.1fs)\n", _objFile.c_str(), seconds);
			cout << "[.stl]: Vertices:  \t" << vertices.size()/3
					<< "\tNormals:   \t" << vertexNormals.size()/3
					<< "\tTex Coords:\t" << vertexTexCoords.size()/2 << endl
				 << "[.stl]: Faces:     \t" << numFaces
					<< "\tTriangles: \t" << numTriangles << endl
				 << "[.stl]: Dimensions:\t(" << (maxX - minX) << ", " << (maxY - minY) << ", " << (maxZ - minZ) << ")" << endl;
			cout << "[.stl]: -=-=-=-=-=-=-=-  END " << _objFile << " Info  -=-=-=-=-=-=-=- " << endl;
		}

		return result;
	}

	//
	//  vector<string> tokenizeString(string input, string delimiters)
	//
	//      This is a helper function to break a single string into std::vector
	//  of strings, based on a given set of delimiter characters.
	//
	vector<string> tokenizeString(string input, string delimiters) {
		if(input.size() == 0)
			return vector<string>();
		
		vector<string> retVec = vector<string>();
		size_t oldR = 0, r = 0; 
		
		//strip all delimiter characters from the front and end of the input string.
		string strippedInput;
		int lowerValidIndex = 0, upperValidIndex = input.size() - 1; 
		while((unsigned int)lowerValidIndex < input.size() && delimiters.find_first_of(input.at(lowerValidIndex), 0) != string::npos)
			lowerValidIndex++;
		
		while(upperValidIndex >= 0 && delimiters.find_first_of(input.at(upperValidIndex), 0) != string::npos)
			upperValidIndex--;
		
		//if the lowest valid index is higher than the highest valid index, they're all delimiters! return nothing.
		if((unsigned int)lowerValidIndex >= input.size() || upperValidIndex < 0 || lowerValidIndex > upperValidIndex)
			return vector<string>();
		
		//remove the delimiters from the beginning and end of the string, if any.
		strippedInput = input.substr(lowerValidIndex, upperValidIndex-lowerValidIndex+1);
		
		//search for each instance of a delimiter character, and create a new token spanning
		//from the last valid character up to the delimiter character.
		while((r = strippedInput.find_first_of(delimiters, oldR)) != string::npos)
		{    
			if(oldR != r)           //but watch out for multiple consecutive delimiters!
				retVec.push_back(strippedInput.substr(oldR, r-oldR));
			oldR = r+1; 
		}    
		if(r != 0)
			retVec.push_back(strippedInput.substr(oldR, r-oldR));
		
		return retVec;
	}

	unsigned char* createTransparentTexture( unsigned char *imageData, unsigned char *imageMask, int texWidth, int texHeight, int texChannels, int maskChannels ) {
		//combine the 'mask' array with the image data array into an RGBA array.
		unsigned char *fullData = new unsigned char[texWidth*texHeight*4];
		
		for(int j = 0; j < texHeight; j++) {
			for(int i = 0; i < texWidth; i++) {
				if( imageData ) {
					fullData[(j*texWidth+i)*4+0] = imageData[(j*texWidth+i)*texChannels+0];	// R
					fullData[(j*texWidth+i)*4+1] = imageData[(j*texWidth+i)*texChannels+1];	// G
					fullData[(j*texWidth+i)*4+2] = imageData[(j*texWidth+i)*texChannels+2];	// B
				} else {
					fullData[(j*texWidth+i)*4+0] = 1;	// R
					fullData[(j*texWidth+i)*4+1] = 1;	// G
					fullData[(j*texWidth+i)*4+2] = 1;	// B
				}
				
				if( imageMask ) {
					fullData[(j*texWidth+i)*4+3] = imageMask[(j*texWidth+i)*maskChannels+0];	// A
				} else {
					fullData[(j*texWidth+i)*4+3] = 1;	// A
				}
			}
		}
		return fullData;
	}

	unsigned char* loadPPM( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path ) {
		FILE *fp;
		
		// make sure the file is there.
		if ((fp = fopen(filename, "rb"))==NULL) {
			string folderName = path + filename;
			if ((fp = fopen(folderName.c_str(), "rb")) == NULL ) {
				if (ERRORS) printf("[.ppm]: [ERROR]: File Not Found: %s\n",filename);
				return 0;
			}
		}
		
		unsigned char *imageData;
		
		int temp, maxValue;
		fscanf(fp, "P%d", &temp);
		if(temp != 3) {
			if (ERRORS) fprintf(stderr, "[.ppm]: [ERROR]: PPM file is not of correct format! (Must be P3, is P%d.)\n", temp);
			fclose(fp);
			return 0;
		}
		
		//got the file header right...
		fscanf(fp, "%d", &texWidth);
		fscanf(fp, "%d", &texHeight);
		fscanf(fp, "%d", &maxValue);
		
		//now that we know how big it is, allocate the buffer...
		imageData = new unsigned char[texWidth*texHeight*3];
		if(!imageData) {
			if (ERRORS) fprintf(stderr, "[.ppm]: [ERROR]: couldn't allocate image memory. Dimensions: %d x %d.\n", texWidth, texHeight);
			fclose(fp);
			return 0;
		}
		
		//and read the data in.
		for(int j = 0; j < texHeight; j++) {
			for(int i = 0; i < texWidth; i++) {
				//read the data into integers (4 bytes) before casting them to unsigned characters
				//and storing them in the unsigned char array.
				int r, g, b;
				fscanf(fp, "%d", &r);
				fscanf(fp, "%d", &g);
				fscanf(fp, "%d", &b);
				
				imageData[(j*texWidth+i)*3+0] = (unsigned char)r;
				imageData[(j*texWidth+i)*3+1] = (unsigned char)g;
				imageData[(j*texWidth+i)*3+2] = (unsigned char)b;
			}
		}
		
		fclose(fp);
		
		texChannels = 3;
		success = true;
		return imageData;
	}

	unsigned char* loadBMP( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path ) {
		FILE *file;
		unsigned long size;                 // size of the image in bytes.
		size_t i;							// standard counter.
		unsigned short int planes;          // number of planes in image (must be 1) 
		unsigned short int bpp;             // number of bits per pixel (must be 24)
		char temp;                          // used to convert bgr to rgb color.
		unsigned char *data;
		
		// make sure the file is there.
		if ((file = fopen(filename, "rb"))==NULL) {
			string folderName = path + filename;
			if ((file = fopen(folderName.c_str(), "rb")) == NULL ) {
				if (ERRORS) printf("[.bmp]: [ERROR]: File Not Found: %s\n",filename);
				return 0;
			}
		}
		
		// seek through the bmp header, up to the width/height:
		fseek(file, 18, SEEK_CUR);
		
		// read the width
		if ((i = fread(&texWidth, 4, 1, file)) != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: reading width from %s.\n", filename);
			return 0;
		}
		//printf("Width of %s: %lu\n", filename, image->sizeX);
		
		// read the height 
		if ((i = fread(&texHeight, 4, 1, file)) != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: reading height from %s.\n", filename);
			return 0;
		}
		//printf("Height of %s: %lu\n", filename, image->sizeY);
		
		// calculate the size (assuming 24 bits or 3 bytes per pixel).
		size = texWidth * texHeight * 3;
		
		// read the planes
		if ((fread(&planes, 2, 1, file)) != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: reading planes from %s.\n", filename);
			return 0;
		}
		if (planes != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: Planes from %s is not 1: %u\n", filename, planes);
			return 0;
		}
		
		// read the bpp
		if ((i = fread(&bpp, 2, 1, file)) != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: reading bpp from %s.\n", filename);
			return 0;
		}
		if (bpp != 24) {
			if (ERRORS) printf("[.bmp]: [ERROR]: Bpp from %s is not 24: %u\n", filename, bpp);
			return 0;
		}
		
		// seek past the rest of the bitmap header.
		fseek(file, 24, SEEK_CUR);
		
		// read the data. 
		data = (unsigned char *) malloc(size);
		if (data == NULL) {
			if (ERRORS) printf("[.bmp]: [ERROR]: allocating memory for color-corrected image data");
			return 0;	
		}
		
		if ((i = fread(data, size, 1, file)) != 1) {
			if (ERRORS) printf("[.bmp]: [ERROR]: reading image data from %s.\n", filename);
			return 0;
		}
		
		for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
			temp = data[i];
			data[i] = data[i+2];
			data[i+2] = temp;
		}

		texChannels = 3;
		success = true;
		return data;
	}

	//
	//  bool loadTGA(string filename, int &imageWidth, int &imageHeight, unsigned char* &imageData)
	//
	//  This function reads RGB (or RGBA) data from the Targa image file specified by filename.
	//  It does not handle color-mapped images -- only RGB/RGBA images, although it correctly supports
	//      both run-length encoding (RLE) for compression and image origins at the top- or bottom-left.
	//
	//  This function fills in imageWidth and imageHeight with the corresponding height and width of 
	//      the image that it reads in from disk, and allocates imageData appropriately. It also sets
	//      wasRGBA to true if the data contained an alpha channel and false if it did not. If the function
	//      completes successfully and wasRGBA is true, then imageData is a pointer to valid image data
	//      that is (imageWidth*imageHeight*4) bytes in size; if wasRBGA is false, it is (imageWidth*imageHeight*3)
	//      bytes large.
	//
	//  If the function fails for any reason, the error message will be sent to stderr, the function will
	//      return false, and imageData will be set to NULL (and deallocated, if the error occurred post-allocation).
	//
	//  The TGA file format, including the data layout with RLE, is based on a spec found at:
	//      http://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
	//
	unsigned char* loadTGA( char* filename, int &texWidth, int &texHeight, int &texChannels, bool &success, bool ERRORS, string path ) {
		unsigned char* imageData;
		
	    FILE *fp;
	    // make sure the file is there.
	    if ((fp = fopen(filename, "rb"))==NULL) {
	    	string folderName = path + filename;
	    	if ((fp = fopen(folderName.c_str(), "rb")) == NULL ) {
	    		if (ERRORS) printf("[.tga]: [ERROR]: File Not Found: %s\n",filename);
	    		return 0;
	    	}
	    }
	    
	    //bunch of data fields in the file header that we read in en masse
	    unsigned char idLength, colorMapType, imageType;
	    unsigned short idxOfFirstColorMapEntry, countOfColorMapEntries;
	    unsigned char numBitsPerColorMapEntry;
	    unsigned short xCoordOfLowerLeft, yCoordOfLowerLeft;
	    unsigned short width, height;
	    unsigned char bytesPerPixel;
	    unsigned char imageAttributeFlags;

	    fread(&idLength, sizeof(unsigned char), 1, fp);
	    fread(&colorMapType, sizeof(unsigned char), 1, fp);
	    fread(&imageType, sizeof(unsigned char), 1, fp);
	    fread(&idxOfFirstColorMapEntry, sizeof(unsigned short), 1, fp);
	    fread(&countOfColorMapEntries, sizeof(unsigned short), 1, fp);
	    fread(&numBitsPerColorMapEntry, sizeof(unsigned char), 1, fp);
	    fread(&xCoordOfLowerLeft, sizeof(unsigned short), 1, fp);
	    fread(&yCoordOfLowerLeft, sizeof(unsigned short), 1, fp);
	    fread(&width, sizeof(unsigned short), 1, fp);
	    fread(&height, sizeof(unsigned short), 1, fp);
	    fread(&bytesPerPixel, sizeof(unsigned char), 1, fp);
	    fread(&imageAttributeFlags, sizeof(unsigned char), 1, fp);

	    //now check to make sure that we actually have the capability to read this file.
	    if(colorMapType != 0) {
	        fprintf(stderr, "[.tga]: Error: TGA file (%s) uses colormap instead of RGB/RGBA data; this is unsupported.\n", filename);
	        imageData = NULL;
	        return 0;
	    }

	    if(imageType != 2 && imageType != 10) {
	        fprintf(stderr, "[.tga]: Error: unspecified TGA type: %d. Only supports 2 (uncompressed RGB/A) and 10 (RLE, RGB/A).\n", imageType); 
	        imageData = NULL;
	        return 0;
	    }

	    if(bytesPerPixel != 24 && bytesPerPixel != 32) {
	        fprintf(stderr, "[.tga]: Error: unsupported image depth (%d bits per pixel). Only supports 24bpp and 32bpp.\n", bytesPerPixel);
	        imageData = NULL;
	        return 0;
	    }

	    //set some helpful variables based on the header information:
	    bool usingRLE = (imageType == 10);              //whether the file uses run-length encoding (compression)
	    bool wasRGBA = (bytesPerPixel == 32);           //whether the file is RGB or RGBA (RGBA = 32bpp)
	    bool topLeft = (imageAttributeFlags & 32);      //whether the origin is at the top-left or bottom-left

	    if( wasRGBA ) {
	    	texChannels = 4;
	    } else {
	    	texChannels = 3;
	    }

	    //this should never happen, since we should never have a color map,
	    //but just in case the data is setting around in there anyway... skip it.
	    if(idLength != 0) {
	        fseek(fp, idLength, SEEK_CUR);
	    }

	    //allocate our image data before we get started.
	    int imageWidth = texWidth;
	    int imageHeight = texHeight;
	    imageData = new unsigned char[imageWidth*imageHeight*(wasRGBA?4:3)];

	    //ok so we can assume at this point that there's no colormap, and 
	    //consequently that the next part of the image is the actual image data.
	    if(usingRLE) {
	        //ok... the data comes in in packets, but we don't know how many of these there'll be.
	        int numberOfPixelsRead = 0;
	        unsigned char dummy;
	        while(numberOfPixelsRead < imageWidth*imageHeight) {
	            //alright let's read the packet header.
	            fread(&dummy, sizeof(unsigned char), 1, fp);
	            bool isRLEPacket = (dummy & 0x80);

	            unsigned char theOtherBitsYesThatWasAPun = (dummy & 0x7F);
	            if(isRLEPacket) {
	                //the other bits (+1) gives the number of times we need to 
	                //repeat the next real set of color values (RGB/A).
	                unsigned char repeatedR, repeatedG, repeatedB, repeatedA;
	                fread(&repeatedR, sizeof(unsigned char), 1, fp);
	                fread(&repeatedG, sizeof(unsigned char), 1, fp);
	                fread(&repeatedB, sizeof(unsigned char), 1, fp);
	                if(wasRGBA) fread(&repeatedA, sizeof(unsigned char), 1, fp);

	                //and go ahead and copy those into the new image. repetitively.
	                for(int i = 0; i < ((int)theOtherBitsYesThatWasAPun+1); i++)
	                {
	                    int idx = numberOfPixelsRead * (wasRGBA ? 4 : 3);
	                    imageData[idx+2] = repeatedR;
	                    imageData[idx+1] = repeatedG;
	                    imageData[idx+0] = repeatedB;
	                    if(wasRGBA) imageData[idx+3] = repeatedA;

	                    numberOfPixelsRead++;
	                }
	            } else {
	                //the other bits (+1) gives the number of consecutive 
	                //pixels we get to read in from the stream willy nilly.
	                for(int i = 0; i < ((int)theOtherBitsYesThatWasAPun+1); i++)
	                {
	                    int idx = numberOfPixelsRead * (wasRGBA ? 4 : 3);
	                    fread(&imageData[idx+2], sizeof(unsigned char), 1, fp);
	                    fread(&imageData[idx+1], sizeof(unsigned char), 1, fp);
	                    fread(&imageData[idx+0], sizeof(unsigned char), 1, fp);
	                    if(wasRGBA) fread(&imageData[idx+3], sizeof(unsigned char), 1, fp);

	                    numberOfPixelsRead++;
	                }
	            }
	        }


	        //and you know what? we're not going to have worried about flipping the image before
	        //if its origin was in the bottom left or top left or whatever. flip it afterwards here if need be.
	        if(!topLeft) {
	            unsigned char *tempCopy = new unsigned char[imageWidth*imageHeight*(wasRGBA?4:3)];
	            for(int i = 0; i < imageHeight; i++) {
	                for(int j = 0; j < imageWidth; j++) {
	                    int copyIdx = (i*imageWidth + j)*(wasRGBA?4:3);
	                    int pullIdx = ((imageHeight - i - 1)*imageWidth + j)*(wasRGBA?4:3);
	                    tempCopy[copyIdx+0] = imageData[pullIdx+0];
	                    tempCopy[copyIdx+1] = imageData[pullIdx+1];
	                    tempCopy[copyIdx+2] = imageData[pullIdx+2];
	                    if(wasRGBA) tempCopy[copyIdx+3] = imageData[pullIdx+3];
	                }
	            }

	            delete imageData;
	            imageData = tempCopy;
	        }

	    } else {

	        //uh well if we're not using run-length encoding, i guess we'll 
	        //just try reading bytes in straight like a normal binary file.
	        unsigned char byte1, byte2, byte3, maybeEvenByte4;
	        for(int i = 0; i < imageHeight; i++) {
	            for(int j = 0; j < imageWidth; j++) {
	                int multiplierThing = wasRGBA ? 4 : 3;

	                //read in the data from file...
	                fread(&byte1, sizeof(unsigned char), 1, fp);
	                fread(&byte2, sizeof(unsigned char), 1, fp);
	                fread(&byte3, sizeof(unsigned char), 1, fp);
	                if(wasRGBA) fread(&maybeEvenByte4, sizeof(unsigned char), 1, fp);

	                //flip the vertical index if the origin is in the bottom-left.
	                int wutHeight = topLeft ? i : (imageHeight - 1 - i);
	                int idx = (wutHeight*imageWidth+j)*multiplierThing;

	                //and load that image into file. seems to be BGR instead of RGB...
	                imageData[idx+2] = byte1;
	                imageData[idx+1] = byte2;
	                imageData[idx+0] = byte3;
	                if(wasRGBA) imageData[idx+3] = maybeEvenByte4;
	            }
	        }
	    }

	    fclose(fp);
	    success = true;
	    return imageData;
	}
