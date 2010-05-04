/* This is a slight adapdation of Fortune's code of sweep algorithm for calculation of
 * Voronoi diagram and Delaunay triangulation. This code has no copyright attached, I belive it is 
 * in public domain, so I don't know if I should attach GPL to that.*/

class triangulation {
public:
	class PointsSource {
	public:
		virtual bool GetNextPoint(float &x, float &y) = 0;
	};

	class TriangleConsumer {
	public:
		virtual void NewTriangle(float x1, float y1, float x2, float y2, float x3, float y3) = 0; 
	};
private:
	typedef struct tagFreenode {
		struct tagFreenode *nextfree;
	} Freenode;

	typedef struct tagFreelist {
		Freenode *head;
		int nodesize;
	} Freelist;

	typedef struct tagPoint {
		float x;
		float y;
	} Point;

/* structure used both for sites and for vertices */

	typedef struct tagSite {
		Point coord;
		int sitenbr;
		int refcnt;
	} Site;

	typedef struct tagEdge {
		float a, b, c;
		Site *ep[2];
		Site *reg[2];
		int edgenbr;
	} Edge;

	typedef struct tagHalfedge {
		struct tagHalfedge *ELleft;
		struct tagHalfedge *ELright;
		Edge *ELedge;
		int ELrefcnt;
		char ELpm;
		Site *vertex;
		float ystar;
		struct tagHalfedge *PQnext;
	} Halfedge;

	int ELhashsize;
	Site *bottomsite;
	Freelist hfl;
	Halfedge *ELleftend, *ELrightend, **ELhash;

	int sorted, triangulate, debug, nsites, siteidx, sqrt_nsites, nvertices, nedges;
	float xmin, xmax, ymin, ymax, deltax, deltay;
	Site *sites;
	Freelist sfl, efl;

	PointsSource* _point_source; 

	TriangleConsumer* _triangle_consumer;

	int PQmin, PQcount, PQhashsize;
	Halfedge *PQhash;

	int ntry, totalsearch;

	Site* readone();
	void ELinitialize(void);
	Halfedge *HEcreate(Edge *, int);
	void ELinsert(Halfedge *, Halfedge *);
	Halfedge *ELgethash(int);
	Halfedge *ELleftbnd(Point *);
	void ELdelete(Halfedge *);
	Halfedge *ELright(Halfedge *);
	Halfedge *ELleft(Halfedge *);
	Site *leftreg(Halfedge *);
	Site *rightreg(Halfedge *);

	void geominit(void);
	Edge *bisect(Site *, Site *);
	Site *intersect(Halfedge *, Halfedge *);
	int right_of(Halfedge *, Point *);
	void endpoint(Edge *, int, Site *);
	float dist(Site *, Site *);
	void makevertex(Site *);
	void deref(Site *);
	void ref(Site *);

	void PQinsert(Halfedge *, Site *, float);
	void PQdelete(Halfedge *);
	int PQbucket(Halfedge *);
	int PQempty(void);
	Point PQ_min(void);
	Halfedge *PQextractmin(void);
	void PQinitialize(void);

	void freeinit(Freelist *, int);
	char *getfree(Freelist *);
	void makefree(Freenode *, Freelist *);
	char *myalloc(unsigned);

	void out_bisector(Edge *);
	void out_ep(Edge *);
	void out_vertex(Site *);
	void out_site(Site *);
	void out_triple(Site *, Site *, Site *);

public:
	triangulation(float _xmin, float _xmax, float _ymin, float _ymax, size_t sites_count, PointsSource* __point_source, TriangleConsumer* __triangle_consumer);
	void go();

};
