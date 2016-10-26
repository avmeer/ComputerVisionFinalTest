#include "stdafx.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include<math.h>

//OpenCV Includes
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/core/opengl.hpp>

//OpenGL and Glut Includes

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glu.h>


#include "Object.h"
#define M_PI   3.14159265358979323846264338327950288
#define KEY_ESCAPE                  27

using namespace cv;
using namespace std;


GLvoid InitGL();
GLvoid OnDisplay();
GLvoid OnReshape(GLint w, GLint h);
GLvoid OnKeyPress(unsigned char key, GLint x, GLint y);
GLvoid OnIdle();


int windowWidth = 512, windowHeight = 512;
GLint g_hWindow;



//our aruco variables
cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_100);
cv::Ptr<cv::aruco::DetectorParameters> detectorParams = cv::aruco::DetectorParameters::create();

std::vector< int > markerIds;
std::vector< std::vector<cv::Point2f> > markerCorners, rejectedCandidates;

cv::VideoCapture cap(0);

double K_[3][3] =
{ { 675, 0, 320 },
{ 0, 675, 240 },
{ 0, 0, 1 } };
cv::Mat K = cv::Mat(3, 3, CV_64F, K_).clone();
const float markerLength = 1.75;
// Distortion coeffs (fill in your actual values here).
double dist_[] = { 0, 0, 0, 0, 0 };
cv::Mat distCoeffs = cv::Mat(5, 1, CV_64F, dist_).clone();
cv::Mat imageMat;

Mat viewMatrix = cv::Mat::zeros(4, 4, CV_32F);

Object *obj;                                // actual object

using namespace std;

int main(int argc, char* argv[])
{

	// Create GLUT Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);

	g_hWindow = glutCreateWindow("Video Texture");

	obj = new Object(argv[1]);

	// Initialize OpenGL
	InitGL();

	glutMainLoop();

	return 0;
}

GLvoid InitGL()
{

	glClearColor(0.0, 0.0, 0.0, 0.0);

	glutDisplayFunc(OnDisplay);
	glutReshapeFunc(OnReshape);
	glutKeyboardFunc(OnKeyPress);
	glutIdleFunc(OnIdle);

}

GLvoid OnDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);


	//Draw the 2D section for the video display
	// Set Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, windowWidth, windowHeight, 0);

	// Switch to Model View Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw a textured quad
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(windowWidth, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(windowWidth, windowHeight);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, windowHeight);
	glEnd();


	//draw the 3d section
	glDisable(GL_TEXTURE_2D);
	float aspectRatio = windowWidth / (float)windowHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, aspectRatio, 0.1, 100000);

	// Capture next frame
	cap >> imageMat; // get image from camera

	IplImage *image;

	cv::aruco::detectMarkers(
		imageMat,		// input image
		dictionary,		// type of markers that will be searched for
		markerCorners,	// output vector of marker corners
		markerIds,		// detected marker IDs
		detectorParams,	// algorithm parameters
		rejectedCandidates);

	if (markerIds.size() > 0) {
		// Draw all detected markers.
		cv::aruco::drawDetectedMarkers(imageMat, markerCorners, markerIds);

		std::vector< cv::Vec3d > rvecs, tvecs;
		cv::aruco::estimatePoseSingleMarkers(
			markerCorners,	// vector of already detected markers corners
			markerLength,	// length of the marker's side
			K,				// input 3x3 floating-point instrinsic camera matrix K
			distCoeffs,		// vector of distortion coefficients of 4, 5, 8 or 12 elements
			rvecs,			// array of output rotation vectors 
			tvecs);			// array of output translation vectors

		for (unsigned int i = 0; i < markerIds.size(); i++) {
			cv::Vec3d r = rvecs[i];
			cv::Vec3d t = tvecs[i];
			if (markerIds[i] == 0) {
				Mat rot;
				Rodrigues(rvecs[i], rot);
				for (unsigned int row = 0; row < 3; ++row)
				{
					for (unsigned int col = 0; col < 3; ++col)
					{
						viewMatrix.at<float>(row, col) = (float)rot.at<double>(row, col);
					}
					viewMatrix.at<float>(row, 3) = (float)tvecs[i][row] * 0.1f;
				}
				viewMatrix.at<float>(3, 3) = 1.0f;

				cv::Mat cvToGl = cv::Mat::zeros(4, 4, CV_32F);
				cvToGl.at<float>(0, 0) = 1.0f;
				cvToGl.at<float>(1, 1) = -1.0f; // Invert the y axis 
				cvToGl.at<float>(2, 2) = -1.0f; // invert the z axis 
				cvToGl.at<float>(3, 3) = 1.0f;
				viewMatrix = cvToGl * viewMatrix;
				cv::transpose(viewMatrix, viewMatrix);

			}

			// Draw coordinate axes.
			cv::aruco::drawAxis(imageMat,
				K, distCoeffs,			// camera parameters
				r, t,					// marker pose
				0.5*markerLength);		// length of the axes to be drawn

										// Draw a symbol in the upper right corner of the detected marker.
		}
	}

	IplImage copy = imageMat;
	image = &copy;


	// Convert to RGB
	cvCvtColor(image, image, CV_BGR2RGB);

	// Create Texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, image->width, image->height, GL_RGB, GL_UNSIGNED_BYTE, image->imageData);




	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float*)viewMatrix.data);
	glScalef(.5, .5, .5);


	static const int coords[6][4][3] = {
		{ { +1, -1, -1 },{ -1, -1, -1 },{ -1, +1, -1 },{ +1, +1, -1 } },
		{ { +1, +1, -1 },{ -1, +1, -1 },{ -1, +1, +1 },{ +1, +1, +1 } },
		{ { +1, -1, +1 },{ +1, -1, -1 },{ +1, +1, -1 },{ +1, +1, +1 } },
		{ { -1, -1, -1 },{ -1, -1, +1 },{ -1, +1, +1 },{ -1, +1, -1 } },
		{ { +1, -1, +1 },{ -1, -1, +1 },{ -1, -1, -1 },{ +1, -1, -1 } },
		{ { -1, -1, +1 },{ +1, -1, +1 },{ +1, +1, +1 },{ -1, +1, +1 } }
	};

	/*for (int i = 0; i < 6; ++i) {
	glColor3ub(i * 20, 100 + i * 10, i * 42);
	glBegin(GL_QUADS);
	for (int j = 0; j < 4; ++j) {
	glVertex3d(0.2*coords[i][j][0], 0.2 * coords[i][j][1], 0.2*coords[i][j][2]);
	}
	glEnd();
	}*/
	obj->draw();




	glFlush();
	glutSwapBuffers();

}


GLvoid OnReshape(GLint w, GLint h)
{
	glViewport(0, 0, w, h);
}

GLvoid OnKeyPress(unsigned char key, int x, int y)
{
	switch (key) {
	case KEY_ESCAPE:
		glutDestroyWindow(g_hWindow);
		exit(0);
		break;
	}
	glutPostRedisplay();
}


GLvoid OnIdle()
{
	// Update View port
	glutPostRedisplay();
}

