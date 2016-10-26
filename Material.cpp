#include "Material.h"

	Material::Material() {
		init( GOL_MATERIAL_BLACK );
	}

	Material::Material( GOL_MaterialColor preDefinedColor ) {
		init( preDefinedColor );
	}

	void Material::init( GOL_MaterialColor preDefinedColor ) {
		this->_illumination = 2;
		
		_ambient[0] = 0.0;	_ambient[1] = 0.0;	_ambient[2] = 0.0;	_ambient[3] = 1.0;
		_diffuse[0] = 0.0;	_diffuse[1] = 0.0;	_diffuse[2] = 0.0;	_diffuse[3] = 1.0;
		_specular[0] = 0.0;	_specular[1] = 0.0;	_specular[2] = 0.0;	_specular[3] = 1.0;
		_emissive[0] = 0.0; _emissive[1] = 0.0;	_emissive[2] = 0.0;	_emissive[3] = 1.0;
		
		_shininess = 0;
		
		switch( preDefinedColor ) {
			case GOL_MATERIAL_WHITE:
				_ambient[0] =  1.0;		_ambient[1] =  1.0;		_ambient[2] =  1.0;
				_diffuse[0] =  1.0;		_diffuse[1] =  1.0;		_diffuse[2] =  1.0;
				_specular[0] = 1.0;		_specular[1] = 1.0;		_specular[2] = 1.0;
				break;
				
			case GOL_MATERIAL_BLACK:
			default:
				break;
				
			case GOL_MATERIAL_BRASS:
				_ambient[0] =  0.33;	_ambient[1] =  0.22;	_ambient[2] =  0.03;
				_diffuse[0] =  0.78;	_diffuse[1] =  0.57;	_diffuse[2] =  0.11;
				_specular[0] = 0.99;	_specular[1] = 0.91;	_specular[2] = 0.81;
				_shininess =  27.8;
				break;
				
			case GOL_MATERIAL_REDPLASTIC:
				_ambient[0] =  0.5;		_ambient[1] =  0.0;		_ambient[2] =  0.0;
				_diffuse[0] =  0.8;		_diffuse[1] =  0.0;		_diffuse[2] =  0.0;
				_specular[0] = 1.0;		_specular[1] = 0.6;		_specular[2] = 0.6;
				_shininess = 32.0;
				break;
				
			case GOL_MATERIAL_GREENPLASTIC:
				_ambient[0] =  0.0;		_ambient[1] =  0.3;		_ambient[2] =  0.0;
				_diffuse[0] =  0.0;		_diffuse[1] =  0.7;		_diffuse[2] =  0.0;
				_specular[0] = 0.6;		_specular[1] = 0.8;		_specular[2] = 0.6;
				_shininess = 32.0;
				break;
				
			case GOL_MATERIAL_CYANRUBBER:
				_ambient[0] =  0.0;		_ambient[1] =  0.05;	_ambient[2] =  0.05;
				_diffuse[0] =  0.4;		_diffuse[1] =  0.5;		_diffuse[2] =  0.5;
				_specular[0] = 0.04;	_specular[1] = 0.7;		_specular[2] = 0.7;
				_shininess = 128.0 * 0.078125;
				break;
		}
	}

	GLfloat* Material::getAmbient() { return _ambient; }
	void Material::setAmbient( GLfloat* amb ) { 
		this->_ambient[0] = amb[0]; 
		this->_ambient[1] = amb[1];
		this->_ambient[2] = amb[2];
		this->_ambient[3] = amb[3];
	}

	GLfloat* Material::getDiffuse() { return _diffuse; }
	void Material::setDiffuse( GLfloat diff[4] ) { 
		this->_diffuse[0] = diff[0]; 
		this->_diffuse[1] = diff[1]; 
		this->_diffuse[2] = diff[2]; 
		this->_diffuse[3] = diff[3]; 
	}

	GLfloat* Material::getSpecular() { return _specular; }
	void Material::setSpecular( GLfloat spec[4] ) { 
		this->_specular[0] = spec[0]; 
		this->_specular[1] = spec[1]; 
		this->_specular[2] = spec[2]; 
		this->_specular[3] = spec[3];
	}

	GLfloat* Material::getEmissive() { return _emissive; }
	void Material::setEmissive( GLfloat emis[4] ) { 
		this->_emissive[0] = emis[0]; 
		this->_emissive[1] = emis[1]; 
		this->_emissive[2] = emis[2]; 
		this->_emissive[3] = emis[3];
	}

	GLfloat Material::getShininess() { return _shininess; }
	void Material::setShininess( GLfloat shin ) { this->_shininess = shin; }

	GLint Material::getIllumination() { return _illumination; }
	void Material::setIllumination( GLint illum ) { this->_illumination = illum; }

	void setCurrentMaterial( Material *material ) {
		GLfloat flat[4] = {0.0, 0.0, 0.0, 1.0};

		if( material->getIllumination() < 1 ) {
			glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, flat );
		} else {
			glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, material->getAmbient() );
		}

		if( material->getIllumination() < 1 ) {
			glColor4fv( material->getDiffuse() );
		} else {
			glColor4f( 1, 1, 1, 1 );
			glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, material->getDiffuse() );
		}

		if( material->getIllumination() < 2 )
			glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, flat );
		else {
			glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, material->getSpecular() );
		}

		glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, material->getEmissive() );

		glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, material->getShininess() );
	}

