#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL\glut.h>
#include "viewer\Arcball.h"                           /*  Arc Ball  Interface         */
#include "Mesh\BaseMesh.h"
#include "MyMesh.h"

using namespace MeshLib;
using namespace std;

void draw_mesh();

/* global mesh */
CBaseMesh<CVertex, CEdge, CFace, CHalfEdge>  mesh;


/*! display call back function
*/
void display()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	draw_mesh();
	glPopMatrix();
	glutSwapBuffers();
}


void draw_mesh()
{
	//画边
	glBegin(GL_LINES);
	for (MeshEdgeIterator<CVertex, CEdge, CFace, CHalfEdge>fiter(&mesh); !fiter.end(); ++fiter)
	{
		CEdge* pf = *fiter;
		CPoint p1 = mesh.edgeVertex1(pf)->point();
		CPoint p2 = mesh.edgeVertex2(pf)->point();
		//常规黑白色(与下面的颜色二选一)
		glColor3f(1, 1, 1);
		glVertex3d(p1[0], p1[1], p1[2]);
		glColor3f(1, 1, 1);
		glVertex3d(p2[0], p2[1], p2[2]);
	}
	glEnd();
}

void init_openGL(int argc, char * argv[])
{
	/* glut stuff */
	glutInit(&argc, argv);                /* Initialize GLUT */
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Mesh Viewer");	  /* Create window with given title */
	glViewport(0, 0, 800, 600);

	glutDisplayFunc(display);             /* Set-up callback functions */

	glutMainLoop();                       /* Start GLUT event-processing loop */
}
void creat_new_mesh()
{
	CVertex *t[3] = { NULL,NULL,NULL };
	t[0] = mesh.createVertex(1);
	t[1] = mesh.createVertex(2);
	t[2] = mesh.createVertex(3);
	CPoint a(-0.5, 0, 0);
	CPoint b(0.5, 0, 0);
	CPoint c(0.5, 0.5, 0);
	t[0]->point() = a;
	t[1]->point() = b;
	t[2]->point() = c;
	mesh.createFace(t, 1);

}
CPoint random_point()
{

	double y = rand() / (RAND_MAX + 1.0) * 0.25;
	double x = rand() / (RAND_MAX + 1.0)*0.5;
	cout << x << " " << y << endl;
	CPoint a(x,y,0);
	return a;
}
void createFace(int faceid, CVertex * cv)
{
	cout << "restructure face : " << faceid << endl;
	CFace *cf = mesh.idFace(faceid);
	CHalfEdge * chf[3] = { NULL,NULL,NULL };	// three halfedges of the faces 
	CVertex * va[3] = { NULL,NULL,NULL };
	chf[0] = cf->halfedge();
	va[0] = chf[0]->source();
	for (int i = 1; i < 3; i++)
	{
		chf[i] = chf[i - 1]->he_next();
		va[i] = chf[i]->source();
	}

	mesh.deleteFace(cf);					// delete the original face

	for (int i = 0; i < 3; i++)
	{
		vector <CVertex *>  v;				// push_back the vertex  in ccw order
		v.push_back(va[i]);
		v.push_back(va[(i + 1) % 3]);
		v.push_back(cv);
		if (i == 0)		CFace *temp = mesh.createFace(v, faceid);
		else    CFace *temp = mesh.createFace(v, mesh.numFaces() + 1);
		//	cout << "num_face :" << mesh.numFaces()  << endl;
		int ans = i < 1 ? faceid : mesh.numFaces();
		cout << "face: " << va[i]->point() << " " << va[(i + 1) % 3]->point() \
			<< " " << cv->point() << " " << ans << endl;
	}

}

bool is_stay_the_edge(CPoint c0, CPoint c1, CPoint c2)
{
	double test_value;
	test_value = c1[0] * c2[1] - c2[0] * c1[1] \
		- (c0[0] * c2[1] - c2[0] * c0[1]) \
		+ c0[0] * c1[1] - c1[0] * c0[1];
	return test_value > 0 ? true : false;
}

int judge_face(CFace *cf, CVertex *p)
{
	CVertex *sour_ver = NULL, *tar_ver = NULL;
	CHalfEdge *che = cf->halfedge();
	for (int i = 0; i < 3; i++)
	{
		sour_ver = che->source();            // assignment of pk,ccw
		tar_ver = che->target();

		bool judge_edge = is_stay_the_edge(p->point(), sour_ver->point(), tar_ver->point()); // judge if the face contains the point

		if (!judge_edge)    // if the value is negative , set the dual halfedge as the next start.
		{
			CFace * cftemp = NULL;
			cftemp = che->he_sym()->face();
			return judge_face(cftemp, p);   // find the target face by recursion 
		}
		che = che->he_next();
	}
	return cf->id();
}

double calc_angle(CHalfEdge *chf)
{
	CPoint p[3];
	CHalfEdge *mhe =chf->he_next()->he_next();	// this is a double circular linked list

	for (int k = 0; k < 3; k++) // preserve three points and  p[0] is the vertex's point
	{
		p[k] = mhe->target()->point();
		mhe = mhe->he_next();

	}
	CPoint temp1, temp2, temp3;
	double len1, len2, len3, b;
	temp1 = p[1] - p[0];
	temp2 = p[2] - p[1];
	temp3 = p[2] - p[0];
	len1 = temp1.norm();	//calculate  norm of vector 
	len2 = temp2.norm();
	len3 = temp3.norm();

	// calculate the angle of this vertex at one face 
	b = (len1 * len1 + len3 * len3 - len2 * len2) / (2 * len1 * len3);
	return acos(b);
}

void adjust_edge(CVertex *cv, int face_id)
{
	CFace * cf = mesh.idFace(face_id);
	CHalfEdge *che = cf->halfedge();
	CHalfEdge *dual = NULL;
	double angle_1, angle_2;

	cout << "face_id:" << face_id << endl;

	while (che->target()->point() == cv->point()
		|| che->source()->point() == cv->point()
		)
	{
		che = che->he_next();
		cout << "halfedge:" << che->target()->point() << " " << che->source()->point() << endl;
	}

	angle_1 = calc_angle(che->he_next()->he_next());
	cout << "angle:" << angle_1 << endl;


	//cout << "halfedge:" << che->target()->point() << " " << che->source()->point() << endl;
	if (che->he_sym() != NULL)
	{
		dual = che->he_sym();
		angle_1 = calc_angle(che->he_next()->he_next());
		angle_2 = calc_angle(dual);
		cout << "angle:" << angle_1 << " " << angle_2 << endl;
	}
}
void insert_point()
{
	CVertex * random_vertex = mesh.createVertex(mesh.numVertices() + 1);
	int sum_face = 0, init_face_id = 0, get_face_id = 0;
	CFace * get_face = NULL;
	CHalfEdge *chf = { NULL };
	random_vertex->point() = random_point();

	sum_face = mesh.numFaces();

	init_face_id = rand() % sum_face + 1;				// randomly generate a point  

	CFace *cf = mesh.idFace(init_face_id);
	get_face_id = judge_face(cf, random_vertex);		// return face which contains the vertex

	createFace(get_face_id, random_vertex);	// create a new face 

	chf = random_vertex->halfedge();		// get a halfedge of this vertex 
	adjust_edge(random_vertex, 1);
	//for (int i = 0; i < 3; i++)
	//{
	//	CHalfEdge * against_he = chf;
	//	while ( against_he->target()->point() == random_vertex->point()
	//		|| against_he->source()->point() == random_vertex->point()
	//		)
	//	{
	//		against_he = against_he->he_next();
	//		cout << "ag:"<<against_he->target()->point()<< " " << against_he->source()->point() << endl;
	//	}
	//	cout << "again : " << against_he->target()->point() << " " << against_he->source()->point() << endl;
	//	//adjust_edge(random_vertex,chf);
	//	chf = chf->he_next();
	//}


	cout << "sum_faces: " << mesh.numFaces() << endl;

}

/*! main function for viewer
*/
int main(int argc, char * argv[])
{
	//srand((unsigned)time(NULL));
	creat_new_mesh();
	for (int i = 0; i < 1; i++)
	{
		//	cout << "--------------" << i << "---------"<<endl;
		insert_point();
	}
	init_openGL(argc, argv);
	return 0;
}
