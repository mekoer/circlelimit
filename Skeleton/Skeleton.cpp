//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Mayer Ádám
// Neptun : XYJP9S
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"
#include "iostream"



// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec3 vp;	// Varying input: vp = vertex position is expected in attrib array 0
	layout(location = 1) in vec3 vertexUV;

	out vec3 texCoord;

	void main() {
		texCoord = vertexUV;
		gl_Position = vec4(vp.x, vp.y, vp.z, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	uniform sampler2D textureUnit;
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

using namespace std;

GPUProgram gpuProgram; // vertex and fragment shaders

enum Animation {
	PLAYING, PAUSED
};
Animation animation = PAUSED;

class Camera {
	mat4 MVP;
	mat4 P;
	mat4 V;
	mat4 animation;

	float size;
	vec3 camCenter;
public:
	Camera() : size(150), camCenter(vec3(20, 30, 0)) {
		float half = 2 / size;
		P = ScaleMatrix(vec3(half, half, 1));
		V = TranslateMatrix(vec3(-1 * camCenter.x, -1 * camCenter.y, 0));

		MVP = V * P;

		int location = glGetUniformLocation(gpuProgram.getId(), "MVP");
		glUniformMatrix4fv(location, 1, GL_TRUE, &MVP[0][0]);
	}

	void orbit(float theta) {
		mat4 transRotPiv = TranslateMatrix(vec3(-50, -30, 0));
		mat4 rotate = RotationMatrix(theta, vec3(0, 0, 1));
		mat4 transOrbPiv = TranslateMatrix(vec3(30, 0, 0));
		mat4 orbit = RotationMatrix(theta, vec3(0, 0, 1));
		mat4 transBack = TranslateMatrix(vec3(20, 30, 0));

		animation = transRotPiv * rotate * transOrbPiv * orbit * transBack;

		updateMVP();
	}

	// call this everytime a matrix gets updated
	void updateMVP() {
		MVP = animation * V * P;
		int location = glGetUniformLocation(gpuProgram.getId(), "MVP");
		glUniformMatrix4fv(location, 1, GL_TRUE, &MVP[0][0]);
	}

	void inverse(vec4 point) {
		mat4 scaleInv = ScaleMatrix(vec3(size / 2, size / 2, size / 2));
		mat4 transInv = TranslateMatrix(vec3(camCenter.x, camCenter.y, 0));

		mat4 invP = scaleInv * transInv;
		vec4 transl = point * invP;
		cout << transl.x << " " << transl.y << endl;
	}
};

class Star {
	unsigned int vao, vbo;
	vector<vec3> vtx;

	vec3 center;
	float s;

public:
	// TODO: vertexuv textura terbeli pontjait is feltolteni a kettes slotba
	Star() : center(vec3(50, 30, 1)), s(40) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		vector<vec3> corners;
		vector<vec3> midpoints;

		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				float x = center.x + (i * 40);
				float y = center.y + (j * 40);
				if (i == 0 || j == 0) {
					if (i == 0 && j == 0) {
						continue;
					}
					else {
						x = center.x + (i * s);
						y = center.y + (j * s);
						midpoints.push_back(vec3(x, y, 1));
					}
				}
				else {
					corners.push_back(vec3(x, y, 1));
				}
			}
		}

		vtx.push_back(vec3(center.x, center.y, center.z));
		vtx.push_back(corners[0]);
		vtx.push_back(midpoints[0]);
		vtx.push_back(corners[1]);
		vtx.push_back(midpoints[2]);
		vtx.push_back(corners[3]);
		vtx.push_back(midpoints[3]);
		vtx.push_back(corners[2]);
		vtx.push_back(midpoints[1]);
		vtx.push_back(corners[0]);
	}

	void addToSval(float incr) {
		s += incr;
		vtx.clear();

		vector<vec3> corners;
		vector<vec3> midpoints;

		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				float x = center.x + (i * 40);
				float y = center.y + (j * 40);
				if (i == 0 || j == 0) {
					if (i == 0 && j == 0) {
						continue;
					}
					else {
						x = center.x + (i * s);
						y = center.y + (j * s);
						midpoints.push_back(vec3(x, y, 1));
					}
				}
				else {
					corners.push_back(vec3(x, y, 1));
				}
			}
		}

		vtx.push_back(vec3(center.x, center.y, center.z));
		vtx.push_back(corners[0]);
		vtx.push_back(midpoints[0]);
		vtx.push_back(corners[1]);
		vtx.push_back(midpoints[2]);
		vtx.push_back(corners[3]);
		vtx.push_back(midpoints[3]);
		vtx.push_back(corners[2]);
		vtx.push_back(midpoints[1]);
		vtx.push_back(corners[0]);
	}

	void draw() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_STATIC_DRAW);
		gpuProgram.setUniform(vec3(0.0f, 1.0f, 1.0f), "color");
		glDrawArrays(GL_TRIANGLE_FAN, 0, vtx.size());

		//glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
		gpuProgram.setUniform(vec3(1.0f, 0.0f, 0.0f), "color");
		glDrawArrays(GL_POINTS, 0, vtx.size());

		//teszt
		vector<vec3> testpoints;
		vec3 origin(0, 0, 1);
		vec3 starcenter(50, 30, 1);
		vec3 transorigin(20, 30, 1);
		testpoints.push_back(origin);
		testpoints.push_back(starcenter);
		testpoints.push_back(transorigin);

		glBufferData(GL_ARRAY_BUFFER, testpoints.size() * sizeof(vec3), &testpoints[0], GL_STATIC_DRAW);
		gpuProgram.setUniform(vec3(0.0f, 1.0f, 0.0f), "color");
		glDrawArrays(GL_POINTS, 0, testpoints.size());
	}
};

class Poincare {
	unsigned int texture;
	vector<vec4> texIm;
	vector<vec2> circles;
public:
	Poincare() {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		// generate
		genPlaceholderTex();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 300, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texIm[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	float lorentz(vec3 v1, vec3 v2) {
		return v1.x * v2.x + v1.y * v2.y - v1.z * v2.z;
	}

	void genPlaceholderTex() {
		for (int i = 0; i < 300; i++) {
			for (int j = 0; j < 300; j++) {
				if (i / 10 == 0 && j / 10 == 0) {
					texIm.push_back(vec4(1.0f, 1.0f, 1.0f, 1.0f));
				}
				else
					texIm.push_back(vec4(0.0f, 0.0f, 0.0f, 1.0f));
			}
		}
	}

	void lineHyperb(float sine, float cosine) {
		// v0 = (cos(phi), sin(phi), 0)
		// 
		// P pont d tavolsagra: P = (0, 0, 1) * cosh(d) + v0 * sinh(d)
		// P-hez iranyvektor:	(P + (0, 0, 1) * sinh(d) + v0 * cosh(d)) - P
		// erre meroleges: v.cross(0, 0, 1)


	}
};

Camera* camera;
Star* star;
Poincare* poincare;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glLineWidth(3);
	glPointSize(10);

	gpuProgram.create(vertexSource, fragmentSource, "outColor");

	star = new Star();
	camera = new Camera();
	//poincare = new Poincare();
	
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	star->draw();

	glutSwapBuffers(); // exchange buffers for double buffering
}

long startingTime = 0;
// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key)
	{
	case 'h':
		star->addToSval(-10);
		glutPostRedisplay();
		break;
	case 'a':
		if (animation == PLAYING) {
			animation = PAUSED;
		}
		else {
			animation = PLAYING;
			startingTime = glutGet(GLUT_ELAPSED_TIME);
		}
		break;
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	//float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	//float cY = 1.0f - 2.0f * pY / windowHeight;
	//printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	switch (button) {
	case GLUT_LEFT_BUTTON:
		printf("Mouse click at (%3.2f, %3.2f)\n", cX, cY);
		camera->inverse(vec4(cX, cY, 1, 0));
		break;
	case GLUT_MIDDLE_BUTTON:  break;
	case GLUT_RIGHT_BUTTON:    break;
	}
}


// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	if (animation == PLAYING) {
		long time = glutGet(GLUT_ELAPSED_TIME) - startingTime;
		//cout << time << endl;
		float theta = (360.0f / 10.0f) * (time / 1000.0f);
		//theta = fmodf(theta, 360);
		theta *= M_PI / 180.0f;

		camera->orbit(theta);

		glutPostRedisplay();
	}
		
	
}
