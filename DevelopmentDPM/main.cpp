#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;
using namespace std;


#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/objloader.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>

#include <DetectMarker.h>

#define PAT_ROW    (7)          /* パターンの行数 */
#define PAT_COL    (10)         /* パターンの列数 */
#define PAT_SIZE   (PAT_ROW*PAT_COL)
#define ALL_POINTS (IMAGE_NUM*PAT_SIZE)
#define CHESS_SIZE (18.5)       /* パターン1マスの1辺サイズ[mm] */

#define RESIZE_ROW (0.2)
#define RESIZE_COL (0.2)

#define AXES_SIZE (50)

int main(void) {
	std::cout << "unko" << endl;
	cv::Mat img,cap;
	ostringstream ostr;
	ostr << "calib/img06.jpg";
	cap = cv::imread(ostr.str());

	cv::resize(cap, img, cv::Size(), RESIZE_COL, RESIZE_ROW);

	cv::Mat rvecs, tvecs, rmats; // 各ビューの回転ベクトルと並進ベクトル
	cv::Mat mycam_mat = (cv::Mat_<double>(3, 3) <<
		895.1076689946736, 0, 564.5335470305324,
		0, 901.9878301364865, 418.3297498865937,
		0.0, 0.0, 1.0);

	cv::Mat mydist_coefs = (cv::Mat_<double>(1, 5) <<
		2.9246680209454490e-01, -1.8921275357895280e+00,
		-6.8383135107349651e-03, 1.2449729555122594e-03,
		3.5656673993194343e+00);

	cv::Size pattern_size = cv::Size2i(PAT_COL, PAT_ROW);
	vector<cv::Point2f> corners;
	vector<vector<cv::Point2f>> img_points;

	vector<cv::Point3f> object;
	for (int i = 0; i < PAT_ROW; i++)
	{
		for (int j = 0; j < PAT_COL; j++)
		{
			cv::Point3f p(
				i * CHESS_SIZE,
				j * CHESS_SIZE,
				0.0);
			object.push_back(p);
		}
	}

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Tutorial 05 - Textured Cube", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test

	//glEnable(GL_DEPTH_TEST);

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 45ｰ Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(0, 0, 1.81), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	//glm::mat4 Model(

	//);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around


	// Create and compile our GLSL program from the shaders
	GLuint programID2 = LoadShaders("SimpleTransform.vertexshader", "SingleColor.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID2 = glGetUniformLocation(programID2, "MVP");

	// ひとつのOpenGLテクスチャを作ります。
	GLuint textureID;
	glGenTextures(1, &textureID);

	// 新たに作られたテクスチャを"バインド"します。つまりここから後のテクスチャ関数はこのテクスチャを変更します。
	glBindTexture(GL_TEXTURE_2D, textureID);

	/* テクスチャ画像はバイト単位に詰め込まれている */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// OpenGLに画像を渡します。
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_BGR, GL_UNSIGNED_BYTE, img2.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");


	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f,-0.75f, 0.0f,
		1.0f,-0.75f, 0.0f,
		-1.0f, 0.75f, 0.0f,
		 1.0f, 0.75f, 0.0f,
		1.0f,-0.75f, 0.0f,
		-1.0f, 0.75f, 0.0f
	};

	// Two UV coordinatesfor each vertex. They were created with Blender.
	static const GLfloat g_uv_buffer_data[] = {
		-1.0f,1.0f,
		0.0f,1.0f,
		-1.0f, 0.0f,
		 0.0f, 0.0f,
		0.0f,1.0f,
		-1.0f, 0.0f
	};

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

	//--------------------物体---------------

	double fx = mycam_mat.at<double>(0, 0);
	double fy = mycam_mat.at<double>(1, 1);
	double cx = mycam_mat.at<double>(0, 2);
	double cy = mycam_mat.at<double>(1, 2);
	double w = img.cols, h = img.rows;
	double zfar = 0.1f;
	double znear = 1000.0f;

	glm::mat4 Projection2(
		2.0 * fx / w, 0, 0, 0,
		0, 2.0 * fy / h, 0, 0,
		1.0 - (2.0 * cx / w), -1.0 + 2.0 * cy / h,  -(zfar + znear) / (zfar - znear), -1.0,
		0, 0,  -2.0 * zfar * znear / (zfar - znear), 0);
	glm::mat4 View2; //動的
	glm::mat4 Model2 = glm::mat4(1.0f);

	glm::mat4 MVP2; //動的

	static const GLfloat g_vertex_buffer_data2[] = {
	0.0f,0.0f,0.0f,
	50.0f,0.0f,0.0f,
	0.0f,0.0f,0.0f,
	0.0f,50.0f,0.0f,
	0.0f,0.0f,0.0f,
	0.0f,0.0f,50.0f
	};

	GLuint vertexbuffer2;
	glGenBuffers(1, &vertexbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data2), g_vertex_buffer_data2, GL_STATIC_DRAW);

	while (1) {
		cv::Mat roopimg;

		//ostringstream ostr;
		//ostr << "calib/img03.jpg";
		roopimg = cv::imread(ostr.str());

		cv::resize(roopimg, roopimg, cv::Size(), RESIZE_COL, RESIZE_ROW);

		bool found = cv::findChessboardCorners(roopimg, pattern_size, corners);
		if (!found) {
			cv::imshow("image", roopimg);
			int key = cv::waitKey(1);
			if (key == 113) {
				break;
			}
			continue;
		}

		cv::solvePnP(object, corners, mycam_mat, mydist_coefs, rvecs, tvecs);
		cv::Rodrigues(rvecs, rmats);

		std::cout << "tvecs" << tvecs << endl;
		std::cout << "rmats" << rmats << endl;

		View2 = glm::mat4(
			rmats.at<double>(0), -rmats.at<double>(3), -rmats.at<double>(6), 0,
			rmats.at<double>(1), -rmats.at<double>(4), -rmats.at<double>(7), 0,
			rmats.at<double>(2), -rmats.at<double>(5), -rmats.at<double>(8), 0,
			tvecs.at<double>(0), -tvecs.at<double>(1), -tvecs.at<double>(2), 1
			);

		MVP2 = Projection2 * View2 * Model2;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 2 * 3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

//-------------------物体-----------------------
		glUseProgram(programID2);

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP2[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);


		// Draw the triangle !
		glDrawArrays(GL_LINES, 0, 6); // 3 indices starting at 0 -> 1 triangle
		glDisableVertexAttribArray(0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


    return 0;
}