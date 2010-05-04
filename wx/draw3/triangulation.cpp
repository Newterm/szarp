#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "triangulation.h"

#define DELETED -2
#define le 0
#define re 1

void triangulation::ELinitialize(void)
{
	int i;

	freeinit(&hfl, sizeof(Halfedge));
	ELhashsize = 2 * sqrt_nsites;
	ELhash = (Halfedge **) myalloc(sizeof(*ELhash) * ELhashsize);
	for (i = 0; i < ELhashsize; i++) {
		ELhash[i] = (Halfedge *) NULL;
	}
	ELleftend = HEcreate((Edge *) NULL, 0);
	ELrightend = HEcreate((Edge *) NULL, 0);
	ELleftend->ELleft = (Halfedge *) NULL;
	ELleftend->ELright = ELrightend;
	ELrightend->ELleft = ELleftend;
	ELrightend->ELright = (Halfedge *) NULL;
	ELhash[0] = ELleftend;
	ELhash[ELhashsize - 1] = ELrightend;
}

triangulation::Halfedge *triangulation::HEcreate(Edge * e, int pm)
{
	Halfedge *answer;

	answer = (Halfedge *) getfree(&hfl);
	answer->ELedge = e;
	answer->ELpm = pm;
	answer->PQnext = (Halfedge *) NULL;
	answer->vertex = (Site *) NULL;
	answer->ELrefcnt = 0;
	return (answer);
}

void triangulation::ELinsert(Halfedge * lb, Halfedge * new_)
{
	new_->ELleft = lb;
	new_->ELright = lb->ELright;
	(lb->ELright)->ELleft = new_;
	lb->ELright = new_;
}

/* Get entry from hash table, pruning any deleted nodes */

triangulation::Halfedge *triangulation::ELgethash(int b)
{
	Halfedge *he;

	if ((b < 0) || (b >= ELhashsize)) {
		return ((Halfedge *) NULL);
	}
	he = ELhash[b];
	if ((he == (Halfedge *) NULL) || (he->ELedge != (Edge *) DELETED)) {
		return (he);
	}
	/* Hash table points to deleted half edge.  Patch as necessary. */
	ELhash[b] = (Halfedge *) NULL;
	if ((--(he->ELrefcnt)) == 0) {
		makefree((Freenode *) he, (Freelist *) & hfl);
	}
	return ((Halfedge *) NULL);
}

triangulation::Halfedge *triangulation::ELleftbnd(Point * p)
{
	int i, bucket;
	Halfedge *he;

	/* Use hash table to get close to desired halfedge */
	bucket = (p->x - xmin) / deltax * ELhashsize;
	if (bucket < 0) {
		bucket = 0;
	}
	if (bucket >= ELhashsize) {
		bucket = ELhashsize - 1;
	}
	he = ELgethash(bucket);
	if (he == (Halfedge *) NULL) {
		for (i = 1; 1; i++) {
			if ((he = ELgethash(bucket - i)) != (Halfedge *) NULL) {
				break;
			}
			if ((he = ELgethash(bucket + i)) != (Halfedge *) NULL) {
				break;
			}
		}
		totalsearch += i;
	}
	ntry++;
	/* Now search linear list of halfedges for the corect one */
	if (he == ELleftend || (he != ELrightend && right_of(he, p))) {
		do {
			he = he->ELright;
		} while (he != ELrightend && right_of(he, p));
		he = he->ELleft;
	} else {
		do {
			he = he->ELleft;
		} while (he != ELleftend && !right_of(he, p));
	}
    /*** Update hash table and reference counts ***/
	if ((bucket > 0) && (bucket < ELhashsize - 1)) {
		if (ELhash[bucket] != (Halfedge *) NULL) {
			(ELhash[bucket]->ELrefcnt)--;
		}
		ELhash[bucket] = he;
		(ELhash[bucket]->ELrefcnt)++;
	}
	return (he);
}

/*** This delete routine can't reclaim node, since pointers from hash
 : table may be present.
 ***/

void triangulation::ELdelete(Halfedge * he)
{
	(he->ELleft)->ELright = he->ELright;
	(he->ELright)->ELleft = he->ELleft;
	he->ELedge = (Edge *) DELETED;
}

triangulation::Halfedge *triangulation::ELright(Halfedge * he)
{
	return (he->ELright);
}

triangulation::Halfedge *triangulation::ELleft(Halfedge * he)
{
	return (he->ELleft);
}

triangulation::Site *triangulation::leftreg(Halfedge * he)
{
	if (he->ELedge == (Edge *) NULL) {
		return (bottomsite);
	}
	return (he->ELpm == le ? he->ELedge->reg[le] : he->ELedge->reg[re]);
}

triangulation::Site *triangulation::rightreg(Halfedge * he)
{
	if (he->ELedge == (Edge *) NULL) {
		return (bottomsite);
	}
	return (he->ELpm == le ? he->ELedge->reg[re] : he->ELedge->reg[le]);
}

void triangulation::geominit(void)
{
	freeinit(&efl, sizeof(Edge));
	nvertices = nedges = 0;
	sqrt_nsites = sqrt(nsites + 4);
	deltay = ymax - ymin;
	deltax = xmax - xmin;
}

triangulation::Edge *triangulation::bisect(Site * s1, Site * s2)
{
	float dx, dy, adx, ady;
	Edge *newedge;

	newedge = (Edge *) getfree(&efl);
	newedge->reg[0] = s1;
	newedge->reg[1] = s2;
	ref(s1);
	ref(s2);
	newedge->ep[0] = newedge->ep[1] = (Site *) NULL;
	dx = s2->coord.x - s1->coord.x;
	dy = s2->coord.y - s1->coord.y;
	adx = dx > 0 ? dx : -dx;
	ady = dy > 0 ? dy : -dy;
	newedge->c = s1->coord.x * dx + s1->coord.y * dy + (dx * dx +
							    dy * dy) * 0.5;
	if (adx > ady) {
		newedge->a = 1.0;
		newedge->b = dy / dx;
		newedge->c /= dx;
	} else {
		newedge->b = 1.0;
		newedge->a = dx / dy;
		newedge->c /= dy;
	}
	newedge->edgenbr = nedges;
	out_bisector(newedge);
	nedges++;
	return (newedge);
}

triangulation::Site *triangulation::intersect(Halfedge * el1, Halfedge * el2)
{
	Edge *e1, *e2, *e;
	Halfedge *el;
	float d, xint, yint;
	int right_of_site;
	Site *v;

	e1 = el1->ELedge;
	e2 = el2->ELedge;
	if ((e1 == (Edge *) NULL) || (e2 == (Edge *) NULL)) {
		return ((Site *) NULL);
	}
	if (e1->reg[1] == e2->reg[1]) {
		return ((Site *) NULL);
	}
	d = (e1->a * e2->b) - (e1->b * e2->a);
	if ((-1.0e-10 < d) && (d < 1.0e-10)) {
		return ((Site *) NULL);
	}
	xint = (e1->c * e2->b - e2->c * e1->b) / d;
	yint = (e2->c * e1->a - e1->c * e2->a) / d;
	if ((e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
	    (e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
	     e1->reg[1]->coord.x < e2->reg[1]->coord.x)) {
		el = el1;
		e = e1;
	} else {
		el = el2;
		e = e2;
	}
	right_of_site = (xint >= e->reg[1]->coord.x);
	if ((right_of_site && (el->ELpm == le)) ||
	    (!right_of_site && (el->ELpm == re))) {
		return ((Site *) NULL);
	}
	v = (Site *) getfree(&sfl);
	v->refcnt = 0;
	v->coord.x = xint;
	v->coord.y = yint;
	return (v);
}

/*** returns 1 if p is to right of halfedge e ***/

int triangulation::right_of(Halfedge * el, Point * p)
{
	Edge *e;
	Site *topsite;
	int right_of_site, above, fast;
	float dxp, dyp, dxs, t1, t2, t3, yl;

	e = el->ELedge;
	topsite = e->reg[1];
	right_of_site = (p->x > topsite->coord.x);
	if (right_of_site && (el->ELpm == le)) {
		return (1);
	}
	if (!right_of_site && (el->ELpm == re)) {
		return (0);
	}
	if (e->a == 1.0) {
		dyp = p->y - topsite->coord.y;
		dxp = p->x - topsite->coord.x;
		fast = 0;
		if ((!right_of_site & (e->b < 0.0)) ||
		    (right_of_site & (e->b >= 0.0))) {
			fast = above = (dyp >= e->b * dxp);
		} else {
			above = ((p->x + p->y * e->b) > (e->c));
			if (e->b < 0.0) {
				above = !above;
			}
			if (!above) {
				fast = 1;
			}
		}
		if (!fast) {
			dxs = topsite->coord.x - (e->reg[0])->coord.x;
			above = (e->b * (dxp * dxp - dyp * dyp))
			    <
			    (dxs * dyp * (1.0 + 2.0 * dxp / dxs + e->b * e->b));
			if (e->b < 0.0) {
				above = !above;
			}
		}
	} else {
/*** e->b == 1.0 ***/

		yl = e->c - e->a * p->x;
		t1 = p->y - yl;
		t2 = p->x - topsite->coord.x;
		t3 = yl - topsite->coord.y;
		above = ((t1 * t1) > ((t2 * t2) + (t3 * t3)));
	}
	return (el->ELpm == le ? above : !above);
}

void triangulation::endpoint(Edge * e, int lr, Site * s)
{
	e->ep[lr] = s;
	ref(s);
	if (e->ep[re - lr] == (Site *) NULL) {
		return;
	}
	out_ep(e);
	deref(e->reg[le]);
	deref(e->reg[re]);
	makefree((Freenode *) e, (Freelist *) & efl);
}

float triangulation::dist(Site * s, Site * t)
{
	float dx, dy;

	dx = s->coord.x - t->coord.x;
	dy = s->coord.y - t->coord.y;
	return (sqrt(dx * dx + dy * dy));
}

void triangulation::makevertex(Site * v)
{
	v->sitenbr = nvertices++;
	out_vertex(v);
}

void triangulation::deref(Site * v)
{
	if (--(v->refcnt) == 0) {
		makefree((Freenode *) v, (Freelist *) & sfl);
	}
}

void triangulation::ref(Site * v)
{
	++(v->refcnt);
}

void triangulation::PQinsert(Halfedge * he, Site * v, float offset)
{
	Halfedge *last, *next;

	he->vertex = v;
	ref(v);
	he->ystar = v->coord.y + offset;
	last = &PQhash[PQbucket(he)];
	while ((next = last->PQnext) != (Halfedge *) NULL &&
	       (he->ystar > next->ystar ||
		(he->ystar == next->ystar &&
		 v->coord.x > next->vertex->coord.x))) {
		last = next;
	}
	he->PQnext = last->PQnext;
	last->PQnext = he;
	PQcount++;
}

void triangulation::PQdelete(Halfedge * he)
{
	Halfedge *last;

	if (he->vertex != (Site *) NULL) {
		last = &PQhash[PQbucket(he)];
		while (last->PQnext != he) {
			last = last->PQnext;
		}
		last->PQnext = he->PQnext;
		PQcount--;
		deref(he->vertex);
		he->vertex = (Site *) NULL;
	}
}

int triangulation::PQbucket(Halfedge * he)
{
	int bucket;

	if (he->ystar < ymin)
		bucket = 0;
	else if (he->ystar >= ymax)
		bucket = PQhashsize - 1;
	else
		bucket = (he->ystar - ymin) / deltay * PQhashsize;
	if (bucket < 0) {
		bucket = 0;
	}
	if (bucket >= PQhashsize) {
		bucket = PQhashsize - 1;
	}
	if (bucket < PQmin) {
		PQmin = bucket;
	}
	return (bucket);
}

int triangulation::PQempty(void)
{
	return (PQcount == 0);
}

triangulation::Point triangulation::PQ_min(void)
{
	Point answer;

	while (PQhash[PQmin].PQnext == (Halfedge *) NULL) {
		++PQmin;
	}
	answer.x = PQhash[PQmin].PQnext->vertex->coord.x;
	answer.y = PQhash[PQmin].PQnext->ystar;
	return (answer);
}

triangulation::Halfedge *triangulation::PQextractmin(void)
{
	Halfedge *curr;

	curr = PQhash[PQmin].PQnext;
	PQhash[PQmin].PQnext = curr->PQnext;
	PQcount--;
	return (curr);
}

void triangulation::PQinitialize(void)
{
	int i;

	PQcount = PQmin = 0;
	PQhashsize = 4 * sqrt_nsites;
	PQhash = (Halfedge *) myalloc(PQhashsize * sizeof *PQhash);
	for (i = 0; i < PQhashsize; i++) {
		PQhash[i].PQnext = (Halfedge *) NULL;
	}
}

triangulation::Site *triangulation::readone()
{
	Site *s = NULL;

	float x, y;
	if (_point_source->GetNextPoint(x, y)){
		s = (Site *) getfree(&sfl);
		s->refcnt = 0;
		s->sitenbr = siteidx++;
		s->coord.x = x;
		s->coord.y = y;
	}
	return s;
}

void triangulation::freeinit(Freelist * fl, int size)
{
	fl->head = (Freenode *) NULL;
	fl->nodesize = size;
}

char *triangulation::getfree(Freelist * fl)
{
	int i;
	Freenode *t;
	if (fl->head == (Freenode *) NULL) {
		t = (Freenode *) myalloc(sqrt_nsites * fl->nodesize);
		for (i = 0; i < sqrt_nsites; i++) {
			makefree((Freenode *) ((char *)t + i * fl->nodesize), fl);
		}
	}
	t = fl->head;
	fl->head = (fl->head)->nextfree;
	return ((char *)t);
}

void triangulation::makefree(Freenode * curr, Freelist * fl)
{
	curr->nextfree = fl->head;
	fl->head = curr;
}

char *triangulation::myalloc(unsigned n)
{
	char *t;
	if ((t = (char*) malloc(n)) == (char *)0) {
		fprintf(stderr,
			"Insufficient memory processing site %d\n",
			siteidx);
		exit(0);
	}
	return (t);
}

void triangulation::out_bisector(Edge * e)
{
	if (!triangulate && !debug) {
		printf("l %f %f %f\n", e->a, e->b, e->c);
	}
	if (debug) {
		printf("line(%d) %gx+%gy=%g, bisecting %d %d\n", e->edgenbr,
		       e->a, e->b, e->c, e->reg[le]->sitenbr,
		       e->reg[re]->sitenbr);
	}
}

void triangulation::out_ep(Edge * e)
{
	if (!triangulate) {
		printf("e %d", e->edgenbr);
		printf(" %d ",
		       e->ep[le] != (Site *) NULL ? e->ep[le]->sitenbr : -1);
		printf("%d\n",
		       e->ep[re] != (Site *) NULL ? e->ep[re]->sitenbr : -1);
	}
}

void triangulation::out_vertex(Site * v)
{
	if (!triangulate && !debug) {
		printf("v %f %f\n", v->coord.x, v->coord.y);
	}
	if (debug) {
		printf("vertex(%d) at %f %f\n", v->sitenbr, v->coord.x,
		       v->coord.y);
	}
}

void triangulation::out_site(Site * s)
{
}

void triangulation::out_triple(Site * s1, Site * s2, Site * s3)
{
	if (triangulate && !debug) {
		_triangle_consumer->NewTriangle(s1->coord.x, s1->coord.y, s2->coord.x, s2->coord.y, s3->coord.x, s3->coord.y);
		printf("%d %d %d\n", s1->sitenbr, s2->sitenbr, s3->sitenbr);
	}
	if (debug) {
		printf("circle through left=%d right=%d bottom=%d\n",
		       s1->sitenbr, s2->sitenbr, s3->sitenbr);
	}
}

/*** implicit parameters: nsites, sqrt_nsites, xmin, xmax, ymin, ymax,
 : deltax, deltay (can all be estimates).
 : Performance suffers if they are wrong; better to make nsites,
 : deltax, and deltay too big than too small.  (?)
 ***/

triangulation::triangulation(float _xmin, float _xmax, float _ymin, float _ymax, size_t sites_count, PointsSource* __point_source, TriangleConsumer* __triangle_consumer)
{
	xmin = _xmin;
	xmax = _xmax;
	ymin = _ymin;
	ymax = _ymax;

	nsites = sites_count;

	geominit();

	freeinit(&sfl, sizeof(Site));

	debug = 0;
	triangulate = 1;

	_point_source = __point_source;
	_triangle_consumer = __triangle_consumer;
	siteidx = 0;
}

void triangulation::go()
{
	Site *newsite, *bot, *top, *temp, *p, *v;
	Point newintstar;
	int pm;
	Halfedge *lbnd, *rbnd, *llbnd, *rrbnd, *bisector;
	Edge *e;

	PQinitialize();
	bottomsite = readone();
	out_site(bottomsite);
	ELinitialize();
	newsite = readone();
	while (1) {
		if (!PQempty()) {
			newintstar = PQ_min();
		}
		if (newsite != (Site *) NULL && (PQempty()
						 || newsite->coord.y < newintstar.y || (newsite->coord.y == newintstar.y && newsite->coord.x < newintstar.x))) {	/* new site is
																					   smallest */
			{
				out_site(newsite);
			}
			lbnd = ELleftbnd(&(newsite->coord));
			rbnd = ELright(lbnd);
			bot = rightreg(lbnd);
			e = bisect(bot, newsite);
			bisector = HEcreate(e, le);
			ELinsert(lbnd, bisector);
			p = intersect(lbnd, bisector);
			if (p != (Site *) NULL) {
				PQdelete(lbnd);
				PQinsert(lbnd, p, dist(p, newsite));
			}
			lbnd = bisector;
			bisector = HEcreate(e, re);
			ELinsert(lbnd, bisector);
			p = intersect(bisector, rbnd);
			if (p != (Site *) NULL) {
				PQinsert(bisector, p, dist(p, newsite));
			}
			newsite = readone();
		} else if (!PQempty()) {	/* intersection is smallest */
			lbnd = PQextractmin();
			llbnd = ELleft(lbnd);
			rbnd = ELright(lbnd);
			rrbnd = ELright(rbnd);
			bot = leftreg(lbnd);
			top = rightreg(rbnd);
			out_triple(bot, top, rightreg(lbnd));
			v = lbnd->vertex;
			makevertex(v);
			endpoint(lbnd->ELedge, lbnd->ELpm, v);
			endpoint(rbnd->ELedge, rbnd->ELpm, v);
			ELdelete(lbnd);
			PQdelete(rbnd);
			ELdelete(rbnd);
			pm = le;
			if (bot->coord.y > top->coord.y) {
				temp = bot;
				bot = top;
				top = temp;
				pm = re;
			}
			e = bisect(bot, top);
			bisector = HEcreate(e, pm);
			ELinsert(llbnd, bisector);
			endpoint(e, re - pm, v);
			deref(v);
			p = intersect(llbnd, bisector);
			if (p != (Site *) NULL) {
				PQdelete(llbnd);
				PQinsert(llbnd, p, dist(p, bot));
			}
			p = intersect(bisector, rrbnd);
			if (p != (Site *) NULL) {
				PQinsert(bisector, p, dist(p, bot));
			}
		} else {
			break;
		}
	}

	for (lbnd = ELright(ELleftend);
	     lbnd != ELrightend; lbnd = ELright(lbnd)) {
		e = lbnd->ELedge;
		out_ep(e);
	}
}
