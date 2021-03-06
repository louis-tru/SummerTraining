// BSP Tips
// October 30, 2000
// by Charles Bloom, cbloom@cbloom.com
// http://www.cbloom.com/3d/techdocs/bsp_tips.txt

// CSG:
// Efficient Boundary Extraction of BSP Solids Based on Clipping Operations

#include "stdafx.h"
#pragma hdrstop
#include <Meshok/Meshok.h>
#include <Meshok/BSP.h>

using namespace BSP;

#define THICK_PLANE_SIDE_EPSILON	(0.25f)
#define NORMAL_PLANE_SIDE_EPSILON	(0.1f)
#define PRECISE_PLANE_SIDE_EPSILON	(0.01f)

/*
-----------------------------------------------------------------------------
	SBspStats
-----------------------------------------------------------------------------
*/
BspStats::BspStats()
{
	this->Reset();
}
void BspStats::Reset()
{
	mxZERO_OUT( *this );
}
void BspStats::Print( UINT32 elapsedTimeMSec )
{
	DBGOUT( "=== BSP statistics ========"			);
	DBGOUT( "Num. Polys(Begin): %u", m_polysBefore	 );
	DBGOUT( "Num. Polys(End):   %u", m_polysAfter		 );
	DBGOUT( "Num. Splits:       %u", m_numSplits			);
	DBGOUT( "Num. Planes:       %u", m_numPlanes );
	DBGOUT( "Num. Inner Nodes:  %u", m_numInternalNodes );
	DBGOUT( "Num. Solid Leaves: %u", m_numSolidLeaves	 );
	DBGOUT( "Num. Empty Leaves: %u", m_numEmptyLeaves	 );
	//DBGOUT( "Tree Depth:        %u", depth			 );
	DBGOUT( "Memory used:		%u KiB", m_bytesAllocated / 1024 );
	DBGOUT( "Time elapsed:      %u msec", elapsedTimeMSec		);
	DBGOUT( "==== End ===================="			 );
}

mxDEFINE_CLASS(BspNode);
mxBEGIN_REFLECTION(BspNode)
	mxMEMBER_FIELD( plane ),
	mxMEMBER_FIELD( front ),
	mxMEMBER_FIELD( back ),
	mxMEMBER_FIELD( polys ),
mxEND_REFLECTION;

mxDEFINE_CLASS(BspVertex);
mxBEGIN_REFLECTION(BspVertex)
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( st ),
mxEND_REFLECTION;

mxDEFINE_CLASS(BspPoly);
mxBEGIN_REFLECTION(BspPoly)
	mxMEMBER_FIELD( vertices ),
	mxMEMBER_FIELD( next ),
mxEND_REFLECTION;

/*
-----------------------------------------------------------------------------
	Tree
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(BspTree);
mxBEGIN_REFLECTION(BspTree)
	mxMEMBER_FIELD( m_nodes ),
	mxMEMBER_FIELD( m_planes ),
	mxMEMBER_FIELD( m_polys ),
mxEND_REFLECTION;
BspTree::BspTree()
{
}

enum {
	MAX_TRACE_PLANES = 32,
	MAX_VERTS_IN_POLY = 32,
};

typedef TStaticList<Vector4,MAX_TRACE_PLANES>	PlaneStack;

// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
//#define	SURFACE_CLIP_EPSILON	(0.125)
#define SURFACE_CLIP_EPSILON	(1/32.0f)

// relation of a polygon to some splitting plane
// used to classify polygons when building a BSP tree
//
enum EPolyStatus
{
	Poly_Front,	// The polygon is lying in front of the plane.
	Poly_Back,	// The polygon is lying in back of the plane.
	Poly_Split,	// The polygon intersects with the plane.
	Poly_Coplanar,	// The polygon is lying on the plane.
};

static inline
EPlaneSide CalculatePlaneSide(
							  const Vector4& plane,
							  const Float3& point,
							  const FLOAT epsilon
							  )
{
	const float distance = Plane_PointDistance( plane, point );
	if ( distance > epsilon ) {
		return PLANESIDE_FRONT;
	} else if ( distance < -epsilon ) {
		return PLANESIDE_BACK;
	} else {
		return PLANESIDE_ON;
	}
}

EPlaneType CalculatePlaneType( const Vector4& plane )
{
	const Float3& N = Plane_CalculateNormal(plane);

	if ( N[0] == 0.0f ) {
		if ( N[1] == 0.0f ) {
			return N[2] > 0.0f ? EPlaneType::PLANETYPE_Z : EPlaneType::PLANETYPE_NEGZ;
		}
		else if ( N[2] == 0.0f ) {
			return N[1] > 0.0f ? EPlaneType::PLANETYPE_Y : EPlaneType::PLANETYPE_NEGY;
		}
		else {
			return EPlaneType::PLANETYPE_ZEROX;
		}
	}
	else if ( N[1] == 0.0f ) {
		if ( N[2] == 0.0f ) {
			return N[0] > 0.0f ? EPlaneType::PLANETYPE_X : EPlaneType::PLANETYPE_NEGX;
		}
		else {
			return EPlaneType::PLANETYPE_ZEROY;
		}
	}
	else if ( N[2] == 0.0f ) {
		return EPlaneType::PLANETYPE_ZEROZ;
	}
	else {
		return EPlaneType::PLANETYPE_NONAXIAL;
	}
}

Float3 GetPolygonCenter( const BspPoly& polygon )
{
	Float3 center = Float3_Zero();
	int numVertices = polygon.vertices.Num();
	for( int i = 0; i < numVertices; i++ ) {
		center += polygon.vertices[i].xyz;
	}
	center *= ( 1.0f / numVertices );
	return center;
}

Vector4 PlaneFromPoints( const Float3& a, const Float3& b, const Float3& c )
{
	// right-hand rule - counterclockwise == front-facing
	Float3 normal = Float3_Cross( b - a, c - a );
	normal = Float3_Normalized( normal );
	float distance = -Float3_Dot( normal, b );
	return Vector4_Set( normal, distance );
}

Vector4 PlaneFromPolygon( const BspPoly& polygon )
{
	mxASSERT(polygon.vertices.Num() >= 3);
	const BspVertex& a = polygon.vertices[0];
	const BspVertex& b = polygon.vertices[1];
	const BspVertex& c = polygon.vertices[2];
	return PlaneFromPoints( a.xyz, b.xyz, c.xyz );
}

inline bool Planes_Equal( const Vector4 &a, const Vector4 &b ) {
	return Float4_Equal( a, b );
}

inline bool Planes_Equal( const Vector4 &a, const Vector4 &b, const FLOAT epsilon ) {
	return Float4_Equal( a, b, epsilon );
}

inline bool Planes_Equal( const Vector4 &a, const Vector4 &b, const FLOAT normalEps, const FLOAT distEps )
{
	if( fabs( a.w - b.w ) > distEps ) {
		return false;
	}
	Float3 normalA = Plane_CalculateNormal(a);
	Float3 normalB = Plane_CalculateNormal(b);
	if( !Float3_Equal(normalA,normalB,normalEps) ) {
		return false;
	}
	return true;
}

static
EPolyStatus ClassifyPolygon(
							const Vector4& plane,
							const BspVertex* verts, const UINT32 numVerts,
							const FLOAT epsilon = 0.013f )
{
	UINT32	numPointsInFront = 0;
	UINT32	numPointsInBack = 0;
	UINT32	numPointsOnPlane = 0;

	for( UINT32 iVertex = 0; iVertex < numVerts; iVertex++ )
	{
		const Float3& point = verts[ iVertex ].xyz;

		const EPlaneSide ePlaneSide = CalculatePlaneSide( plane, point, epsilon );

		if( ePlaneSide == EPlaneSide::PLANESIDE_FRONT )
		{
			numPointsInFront++;
		}
		if( ePlaneSide == EPlaneSide::PLANESIDE_BACK )
		{
			numPointsInBack++;
		}
		if( ePlaneSide == EPlaneSide::PLANESIDE_ON )
		{
			numPointsOnPlane++;
		}
	}

	if( numVerts == numPointsInFront ) {
		return Poly_Front;
	}
	if( numVerts == numPointsInBack ) {
		return Poly_Back;
	}
	if( numVerts == numPointsOnPlane ) {
		return Poly_Coplanar;
	}
	return Poly_Split;
}

#if 0

enum {
	MAX_TRACE_PLANES = 32,
	MAX_VERTS_IN_POLY = 32,
};

//typedef TStaticList<Vector4,MAX_TRACE_PLANES>	PlaneStack;

/*
Sutherland-Hodgman clipping algorithm:
- For each clip plane:
-  Traverse each line segment in polygon:
-  Test against the plane
- Add a vertex if line crosses the plane
See: Splitting a polygon with a plane
http://ezekiel.vancouver.wsu.edu/~cs442/archive/lectures/bsp-trees/poly-split.pdf
*/
EPlaneSide SplitConvexPolygonByPlane(
							   const BspPoly& polygon,
							   BspPoly &front,
							   BspPoly &back,
							   const Vector4 &plane,
							   const float epsilon
							   )
{
	const UINT32 numPoints = polygon.vertices.Num();
	const Float3* points = polygon.vertices.ToPtr();

	FLOAT	dists[MAX_VERTS_IN_POLY+4];
	UINT8	sides[MAX_VERTS_IN_POLY+4];

	UINT32		counts[3];
	counts[PLANESIDE_FRONT] = 0;
	counts[PLANESIDE_BACK] = 0;
	counts[PLANESIDE_ON] = 0;

	for( UINT32 i = 0; i < numPoints; i++ )
	{
		FLOAT dist = Plane_PointDistance( plane, points[i] );
		if ( dist > epsilon ) {
			sides[i] = PLANESIDE_FRONT;
		} else if ( dist < -epsilon ) {
			sides[i] = PLANESIDE_BACK;
		} else {
			sides[i] = PLANESIDE_ON;
		}
		counts[sides[i]]++;
	}

	if( !counts[PLANESIDE_FRONT] && !counts[PLANESIDE_BACK] ) {
		return PLANESIDE_ON;
	}
	if( !counts[PLANESIDE_FRONT] ) {
		back = polygon;
		return PLANESIDE_BACK;
	}
	if( !counts[PLANESIDE_BACK] ) {
		front = polygon;
		return PLANESIDE_FRONT;
	}

	for( UINT32 i = 0; i < numPoints; i++ )
	{
		// Consider the edge from this vertex to the next vertex in the list, which defines a segment
		UINT32 iPointA = i;
		UINT32 iPointB = (i + 1) % numPoints;
		Float3 pointA = points[iPointA];
		Float3 pointB = points[iPointB];
		UINT32 sideA = sides[iPointA];
		UINT32 sideB = sides[iPointB];
		FLOAT distA = dists[iPointA];
		FLOAT distB = dists[iPointB];

		// If the segment crosses the plane, make a new vertex where the segment intersects the plane.
		if( sideA == PLANESIDE_BACK && sideB == PLANESIDE_FRONT ) {
			// generate a split point
			FLOAT t = distB / (distB - distA);
			Float3 mid = pointA + (pointB - pointA) * t;
			front.vertices.Add( mid );
			back.vertices.Add( mid );
		}
		if( sideB == PLANESIDE_BACK && sideA == PLANESIDE_FRONT ) {
			FLOAT t = distA / (distA - distB);
			Float3 mid = pointB + (pointA - pointB) * t;
			front.vertices.Add( mid );
			back.vertices.Add( mid );
		}

		if( sideB == PLANESIDE_BACK ) {
			back.vertices.Add( pointB );
		} else if( sideB == PLANESIDE_FRONT ) {
			front.vertices.Add( pointB );
		} else {
			back.vertices.Add( pointB );
			front.vertices.Add( pointB );
		}
	}

	return PLANESIDE_CROSS;
}
#endif

mxSWIPED("Doom 3 BFG Edition GPL Source Code, idSoftware");
EPlaneSide SplitConvexPolygonByPlane(
									 const BspPoly& polygon,
									 BspPoly &front,	// valid only if the polygon was split
									 BspPoly &back,		// valid only if the polygon was split
									 const Vector4& plane,
									 const float epsilon
							   )
{
	const int numPoints = polygon.vertices.Num();
	const BspVertex* p = polygon.vertices.ToPtr();

	float *	dists = (float *) _alloca( (numPoints+4) * sizeof( float ) );
	BYTE *	sides = (BYTE *) _alloca( (numPoints+4) * sizeof( BYTE ) );

	int		counts[3]; // 'front', 'back' and 'on'
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	int k;
	for ( k = 0; k < numPoints; k++ ) {
		FLOAT dist = Plane_PointDistance( plane, p[k].xyz );
		dists[k] = dist;
		if ( dist > epsilon ) {
			sides[k] = PLANESIDE_FRONT;
		} else if ( dist < -epsilon ) {
			sides[k] = PLANESIDE_BACK;
		} else {
			sides[k] = PLANESIDE_ON;
		}
		counts[sides[k]]++;
	}
	sides[k] = sides[0];
	dists[k] = dists[0];

	const Float3& planeNormal = Vector4_As_Float3( plane );
	const float planeDistance = -plane.w;

	// if the polygon is aligned with the splitting plane
	if ( counts[PLANESIDE_ON] == numPoints ) {
		return PLANESIDE_ON;
	}
	/** /
	// if coplanar, put on the front side if the normals match
	if ( !counts[PLANESIDE_FRONT] && !counts[PLANESIDE_BACK] )
	{
		const Vector4 polygonPlane = PlaneFromPolygon( polygon );
		const Float3& polygonNormal = Vector4_As_Float3( polygonPlane );
		if ( polygonNormal * planeNormal > 0.0f ) {
			//front = polygon;
			return PLANESIDE_FRONT;
		} else {
			//back = polygon;
			return PLANESIDE_BACK;
		}
	}
	/**/
	// if nothing at the front of the clipping plane
	if ( !counts[PLANESIDE_FRONT] ) {
		//back = polygon;
		return PLANESIDE_BACK;	// the polygon is completely behind the splitting plane
	}
	// if nothing at the back of the clipping plane
	if ( !counts[PLANESIDE_BACK] ) {
		//front = polygon;
		return PLANESIDE_FRONT;	// the polygon is completely in front of the splitting plane
	}

	// Straddles the splitting plane - we must clip.

	// Estimate the maximum number of points.
	const int maxpts = numPoints+4;	// cant use counts[0]+2 because of fp grouping errors

	for ( int i = 0; i < numPoints; i++)
	{
		const BspVertex* p1 = &p[i];

		if ( sides[i] == PLANESIDE_ON ) {
			front.vertices.Add( *p1 );
			back.vertices.Add( *p1 );
			continue;
		}

		if ( sides[i] == PLANESIDE_FRONT ) {
			front.vertices.Add( *p1 );
		}

		if ( sides[i] == PLANESIDE_BACK ) {
			back.vertices.Add( *p1 );
		}

		if ( sides[i+1] == PLANESIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// generate a split point
		const BspVertex* p2 = &p[(i+1)%numPoints];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		BspVertex mid;
		if ( sides[i] == PLANESIDE_FRONT ) {
			float dot = dists[i] / ( dists[i] - dists[i+1] );
			for ( int j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( planeNormal[j] == 1.0f ) {
					mid.xyz[j] = planeDistance;
				} else if ( planeNormal[j] == -1.0f ) {
					mid.xyz[j] = -planeDistance;
				} else {
					mid.xyz[j] = (*p1).xyz[j] + dot * ( (*p2).xyz[j] - (*p1).xyz[j] );
				}
			}
			const Float2 p1st = Half2_To_Float2( p1->st );
			const Float2 p2st = Half2_To_Float2( p2->st );
			mid.st = Float2_To_Half2( p1st + ( p2st - p1st ) * dot );
		} else {
			float dot = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( int j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( planeNormal[j] == 1.0f ) {
					mid.xyz[j] = planeDistance;
				} else if ( planeNormal[j] == -1.0f ) {
					mid.xyz[j] = -planeDistance;
				} else {
					mid.xyz[j] = (*p2).xyz[j] + dot * ( (*p1).xyz[j] - (*p2).xyz[j] );
				}
			}
			const Float2 p1st = Half2_To_Float2( p1->st );
			const Float2 p2st = Half2_To_Float2( p2->st );
			mid.st = Float2_To_Half2( p2st + ( p1st - p2st ) * dot );
		}

		front.vertices.Add( mid );
		back.vertices.Add( mid );
	}

	if ( front.vertices.Num() > maxpts || back.vertices.Num() > maxpts ) {
		ptERROR( "SplitConvexPolygonByPlane: points exceeded estimate." );
	}

	return PLANESIDE_CROSS;
}

/*
The selection of the base polygon and the partitioning plane
is the crucial part of the BSP-tree construction. Depend-
ing on criteria for the base polygon selection different BSP-
trees can be obtained. In our work we use two different ap-
proaches: "naive" selection, where the polygon is randomly
selected from the polygon list, and "optimized" selection.
The optimization means using selection criteria that allow
for obtaining a tree with the following properties:
- Minimization of polygon splitting operations to reduce
the total number of nodes and the number of operations
in the function evaluation
- Minimization of computational errors during the function
evaluation and BSP-tree construction;
- Balancing the BSP tree, i.e., minimization of difference
between positive and negative list for the minimization of
the depth of the tree.
*/
// function for picking an optimal partitioning plane.
// we have two conflicting goals:
// 1) keep the tree balanced
// 2) avoid splitting the polygons
// and avoid introducing new partitioning planes
//
static
BspPolyID FindBestSplitterIndex( const BspTree& tree, const BspPolyID polygons,
							 const SplittingCriteria& options = SplittingCriteria() )
{
	mxASSERT(polygons != BSP_NONE);

	INT		numFrontFaces = 0;
	INT		numBackFaces = 0;
	INT		numSplitFaces = 0;
	INT		numCoplanarFaces = 0;

	BspPolyID	bestSplitter = 0;
	FLOAT	bestScore = 1e6f;	// the less value the better

	BspPolyID iPolyA = polygons;

	while( iPolyA != BSP_NONE )
	{
		// select potential splitter
		const BspPoly& polygonA = tree.m_polys[ iPolyA ];

		// potential splitting plane
		const Vector4 planeA = PlaneFromPolygon( polygonA );

		// test other polygons against the potential splitter
		BspPolyID iPolyB = polygons;
		while( iPolyB != BSP_NONE )
		{
			const BspPoly& polygonB = tree.m_polys[ iPolyB ];
			if( iPolyA != iPolyB )
			{
				// evaluate heuristic cost and select the best candidate

				const EPolyStatus planeSide = ClassifyPolygon(planeA, polygonB.vertices.ToPtr(), polygonB.vertices.Num(), options.planeEpsilon);

				switch( planeSide )
				{
				case Poly_Front :		numFrontFaces++;	break;
				case Poly_Back :		numBackFaces++;		break;
				case Poly_Coplanar :	numCoplanarFaces++;	break;
				case Poly_Split :		numSplitFaces++;	break;
				default:				mxUNREACHABLE;
				}

				// diff == 0 => tree is perfectly balanced
				const UINT32 diff = Abs<INT>( numFrontFaces - numBackFaces );

				FLOAT32 score = (diff * options.balanceVsCuts)
					+ (numSplitFaces * options.splitCost) * (1.0f - options.balanceVsCuts)
					;

				if( CalculatePlaneType(planeA) < PLANETYPE_TRUEAXIAL )
				{
					score *= 0.8f;	// axial is better
				}

				// A smaller score will yield a better tree.
				if( score < bestScore )
				{
					bestScore = score;
					bestSplitter = iPolyA;
				}
			}
			iPolyB = polygonB.next;
		}//for all tested polygons
		iPolyA = polygonA.next;
	}//for all potential splitters

	mxASSERT(bestSplitter != BSP_NONE);
	return bestSplitter;
}

#define	NORMAL_EPSILON		0.00001f
#define	DIST_EPSILON		0.01f

static UINT32 GetPlaneIndex(
				   BspTree & tree, const Vector4& plane,
				   const float normalEps = NORMAL_EPSILON,
				   const float distEps = DIST_EPSILON
				   )
{
	Vector4	normalizedPlane = plane;
	//normalizedPlane.FixDegeneracies( distEps );

	mxASSERT( distEps <= 0.125f );

	const UINT32 numExistingPlanes = tree.m_planes.Num();

	for( UINT32 iPlane = 0; iPlane < numExistingPlanes; iPlane++ )
	{
		const Vector4 existingPlane = tree.m_planes[ iPlane ];

		if( Planes_Equal( existingPlane, normalizedPlane, normalEps, distEps ) )
		{
			return iPlane;
		}
	}

	const UINT32 newPlaneIndex = numExistingPlanes;
	mxASSERT( newPlaneIndex <= BSP_MAX_PLANES );

	tree.m_planes.Add( normalizedPlane );

	return newPlaneIndex;
}

static inline BspNodeID NewNode( BspTree & tree )
{
	const UINT32 newNodeIndex = tree.m_nodes.Num();
	mxASSERT( newNodeIndex <= BSP_MAX_NODES );
	BspNode & newNode = tree.m_nodes.Add();
#if MX_DEBUG
	memset(&newNode,-1,sizeof BspNode);
	//DBGOUT("! creating node %u",newNodeIndex);
#endif //MX_DEBUG
	return newNodeIndex;
}

static BspPolyID AddPolygon( const BspPoly& poly, BspTree & tree, BspPolyID * head )
{
	const UINT32 newPolyIndex = tree.m_polys.Num();
	mxASSERT( newPolyIndex <= BSP_MAX_POLYS );
	BspPoly &newPoly = tree.m_polys.Add();
	newPoly.vertices.SetExternalStorage( newPoly.buffer, mxCOUNT_OF(newPoly.buffer) );
	newPoly.vertices = poly.vertices;
	newPoly.next = *head;
	*head = newPolyIndex;
	return newPolyIndex;
}

static int CalculatePolygonCount( const BspTree& tree, BspPolyID iPoly )
{
	int result = 0;
	while( iPoly != BSP_NONE )
	{
		const BspPoly&	polygon = tree.m_polys[ iPoly ];
		result++;
		iPoly = polygon.next;
	}
	return result;
}

// returns index of the splitting plane
//
static UINT32 PartitionPolygons(
					   BspTree & tree,
					   const int nodeIndex,
					   BspStats &stats,
					   const BspPolyID polygons,
					   BspPolyID *frontPolys,
					   BspPolyID *backPolys,
					   const FLOAT epsilon = 0.13f
					   )
{
	// select the best partitioner
	SplittingCriteria	settings;
	// we don't need polygons for collision detection
	settings.splitCost = 0;
	settings.balanceVsCuts = 1;
	settings.planeEpsilon = 0.1f;

	const BspPolyID bestSplitter = FindBestSplitterIndex( tree, polygons, settings );
	const Vector4 splittingPlane = PlaneFromPolygon( tree.m_polys[ bestSplitter ] );

	BspNode& splittingNode = tree.m_nodes[ nodeIndex ];

	int front = 0, back = 0, split = 0, coplanar = 0;

	// partition the list

	BspPolyID iPoly = polygons;
	while( iPoly != BSP_NONE )
	{
		BspPoly &	polygon = tree.m_polys[ iPoly ];
		const BspPolyID iNextPoly = polygon.next;

		if( iPoly != bestSplitter )
		{
			BspVertex	buffer1[64];
			BspVertex	buffer2[64];

			BspPoly		frontPoly;
			BspPoly		backPoly;
			frontPoly.vertices.SetExternalStorage( buffer1, mxCOUNT_OF(buffer1) );
			backPoly.vertices.SetExternalStorage( buffer2, mxCOUNT_OF(buffer2) );

			const EPlaneSide side = SplitConvexPolygonByPlane( polygon, frontPoly, backPoly, splittingPlane, epsilon );

			if( side == PLANESIDE_CROSS )
			{
				mxASSERT( frontPoly.vertices.Num() && backPoly.vertices.Num() );
				AddPolygon( frontPoly, tree, frontPolys );
				AddPolygon( backPoly, tree, backPolys );
				stats.m_numSplits++;
				split++;
			}
			else if( side == PLANESIDE_FRONT )
			{
				//mxASSERT( frontPoly.vertices.Num() );
				polygon.next = *frontPolys;
				*frontPolys = iPoly;
				front++;
			}
			else if( side == PLANESIDE_BACK )
			{
				//mxASSERT( backPoly.vertices.Num() );
				polygon.next = *backPolys;
				*backPolys = iPoly;
				back++;
			}
			else
			{
				mxASSERT( side == PLANESIDE_ON );
				polygon.next = splittingNode.polys;
				splittingNode.polys = iPoly;
				coplanar++;
			}
		}
		// continue
		iPoly = iNextPoly;
	}

	//LogStream(LL_Debug) << "front: " << front << ", back: " << back << ", split: " << split << ", on: " << coplanar;

	return GetPlaneIndex( tree, splittingPlane );
}

// returns index of new node
//
static BspNodeID BuildTree_R( BspTree & tree, const UINT16 polygons, BspStats &stats )
{
	mxASSERT( polygons != BSP_NONE );

	// allocate a new internal node
	const BspNodeID iNewNode = NewNode( tree );

	// partition the list
	BspPolyID	frontPolys = BSP_NONE;
	BspPolyID	backPolys = BSP_NONE;
	const UINT32 splitPlane = PartitionPolygons( tree, iNewNode, stats, polygons, &frontPolys, &backPolys );

	tree.m_nodes[ iNewNode ].plane = splitPlane;

	// recursively process children
	if( frontPolys != BSP_NONE )
	{
		tree.m_nodes[ iNewNode ].front = BuildTree_R( tree, frontPolys, stats );
	}
	else
	{
		tree.m_nodes[ iNewNode ].front = BSP_EMPTY_LEAF;
		stats.m_numEmptyLeaves++;
	}

	if( backPolys != BSP_NONE )
	{
		tree.m_nodes[ iNewNode ].back = BuildTree_R( tree, backPolys, stats );
	}
	else
	{
		tree.m_nodes[ iNewNode ].back = BSP_SOLID_LEAF;
		stats.m_numSolidLeaves++;
	}

	return iNewNode;
}

ERet BspTree::Build( ATriangleMeshInterface* triangleMesh )
{
	const UINT32 startTimeMSec = mxGetTimeInMilliseconds();

	struct CollectTriangles : ATriangleIndexCallback
	{
		BspTree &	m_tree;
		CollectTriangles( BspTree & tree ) : m_tree( tree )
		{}
		virtual void ProcessTriangle( const Vertex& a, const Vertex& b, const Vertex& c ) override
		{
			BspPoly & triangle = m_tree.m_polys.Add();
			triangle.vertices.SetNum(3);
			BspVertex & v1 = triangle.vertices[0];
			BspVertex & v2 = triangle.vertices[1];
			BspVertex & v3 = triangle.vertices[2];
			v1.xyz = a.xyz;
			v2.xyz = b.xyz;
			v3.xyz = c.xyz;
			v1.st = Float2_To_Half2(a.st);
			v2.st = Float2_To_Half2(b.st);
			v3.st = Float2_To_Half2(c.st);
			triangle.next = BSP_NONE;
		}
	};

	CollectTriangles	callback( *this );
	triangleMesh->ProcessAllTriangles( &callback );

	chkRET_X_IF_NOT(m_polys.Num() > 0, ERR_INVALID_PARAMETER);

	// setup a linked list of polygons
	for( int iPoly = 0; iPoly < m_polys.Num()-1; iPoly++ )
	{
		m_polys[iPoly].next = iPoly+1;
	}

	BspStats	stats;
	stats.m_polysBefore = m_polys.Num();

	BuildTree_R( *this, 0, stats );

	stats.m_polysAfter = m_polys.Num();

	m_nodes.Shrink();
	m_planes.Shrink();
	m_polys.Shrink();

	stats.m_numInternalNodes = m_nodes.Num();
	stats.m_numPlanes = m_planes.Num();
	stats.m_bytesAllocated = this->BytesAllocated();

	const UINT32 currentTimeMSec = mxGetTimeInMilliseconds();
	stats.Print(currentTimeMSec - startTimeMSec);

	return ALL_OK;
}

bool BspTree::PointInSolid( const Float3& point, float epsilon ) const
{
	int nodeIndex = 0;
	// If < 0, we are in a leaf node
	while( nodeIndex >= 0 )
	{
		// Find which side of the node we are on
		const BspNode& node = m_nodes[ nodeIndex ];
		const Vector4& plane = m_planes[ node.plane ];
		const EPlaneSide side = CalculatePlaneSide( plane, point, epsilon );
		// Go down the appropriate side
		nodeIndex = (side == PLANESIDE_FRONT) ? (INT16)node.front : (INT16)node.back;
	}
	return nodeIndex == BSP_SOLID_LEAF;
}

float BspTree::DistanceToPoint( const Float3& point, float epsilon ) const
{
	int nodeIndex = 0;
	float distance = 0.f;
	// If < 0, we are in a leaf node
	while( nodeIndex >= 0 )
	{
		// Find which side of the node we are on
		const BspNode& node = m_nodes[ nodeIndex ];
		const Vector4& plane = m_planes[ node.plane ];
		distance = Plane_PointDistance( plane, point );
		const int nearIndex = (distance >= 0.f);	// child of Node for half-space containing the origin of Ray: 1 - front, 0 - back
		const int sides[2] = { (INT16)node.back, (INT16)node.front };
		// Go down the appropriate side
		if( distance > +epsilon ) {
			nodeIndex = (INT16)node.front;
		} else if( distance < -epsilon ) {
			nodeIndex = (INT16)node.back;
		} else {
			nodeIndex = sides[ nearIndex ];
		}
	}
	// return the minimum (closest) distance
	return distance;
}

bool PlaneLineIntersection( const Float4& plane, const Float3& start, const Float3& end, float *fraction )
{
	const float d1 = Plane_PointDistance( plane, start );
	const float d2 = Plane_PointDistance( plane, end );
	if ( d1 == d2 ) {
		return false;
	}
	if ( d1 > 0.0f && d2 > 0.0f ) {
		return false;
	}
	if ( d1 < 0.0f && d2 < 0.0f ) {
		return false;
	}
	*fraction = ( d1 / ( d1 - d2 ) );
	return ( *fraction >= 0.0f && *fraction <= 1.0f );
}

bool PlaneRayIntersection( const Float4& plane, const Float3& start, const Float3& dir, float *scale )
{
	const Float3& planeNormal = Plane_CalculateNormal( plane );
	const float D = plane.w;

	const float d2 = planeNormal * dir;
	if ( d2 == 0.0f ) {
		return false;
	}

	const float d1 = planeNormal * start + D;

	*scale = -( d1 / d2 );
	return true;
}

size_t BspTree::BytesAllocated() const
{
	return m_nodes.GetAllocatedMemory()
		+ m_planes.GetAllocatedMemory()
		+ m_polys.GetAllocatedMemory()
		;
}

// @todo:
// weld coplanar polygon faces

//// fix degenerate normal and dist
//bool FixDegeneracies( Vector4 * plane FLOAT distEpsilon )
//{
//	bool fixedNormal = FixDegenerateNormal();
//	// only fix dist if the normal was degenerate
//	if ( fixedNormal ) {
//		if ( fabs( d - Math::Rint( d ) ) < distEpsilon ) {
//			d = Math::Rint( d );
//		}
//	}
//	return fixedNormal;
//}


float BspTree::CastRay( const Float3& start, const Float3& direction ) const
{
	int nodeIndex = 0;
	Float3 end = start + direction * 1e4f;

	float distance = 0.f;

	const float epsilon = 1e-3f;

	for(;;)
	{
		// If < 0, we are in a leaf node
		if( nodeIndex < 0 ){
			// return the minimum (closest) distance
			return distance;
		}
		const BspNode& node = m_nodes[ nodeIndex ];
		const Vector4& plane = m_planes[ node.plane ];
		// distance from plane for trace start and end
		const float d1 = Plane_PointDistance( plane, start );
		const float d2 = Plane_PointDistance( plane, end );
		// see which sides we need to consider
		if( d1 >= +epsilon && d2 >= +epsilon ) {
			nodeIndex = (INT16)node.front;
		}
		else if( d1 < -epsilon && d2 < -epsilon ) {
			nodeIndex = (INT16)node.back;
		}
		else {
			// Straddling
			float fraction;
			PlaneLineIntersection( plane, start, end, &fraction );
			const Float3 mid = start + (end - start) * fraction;
			UNDONE;
			CastRay( start, mid );
			CastRay( mid, end );


		}
	}
	mxUNREACHABLE;
	return distance;

#if 0
	TStaticList< BspNode* >	nodes;
	TStaticList< float >	dists;

	float	tmin = 0.0f;
	float	tmax = 1e5f;

	int nodeIndex = 0;
	float distance = 0.f;

	for(;;)
	{
		if( nodeIndex >= 0 )
		{
			const BspNode& node = m_nodes[ nodeIndex ];
			const Vector4& plane = m_planes[ node.plane ];
			const Float3& planeNormal = Vector4_As_Float3( plane );
			const float denom = Float3_Dot(direction, planeNormal);
			const float distance = Plane_PointDistance( plane, start );
			const int nearIndex = (distance > 0.f);
			// If denom is zero, ray runs parallel to the plane.
			// In this case, just fall through to visit the near side
			// (the one 'start' lies on).
			if( denom != 0.0f )
			{
				float t = distance / denom;
				if( 0.f <= t && t <= tmax )
				{
					if( t >= tmin )
					{
						// Straddling, push far side onto stack, then visit near side
						UNDONE;
					}
				}
			}
		}


		UNDONE;
	}
	mxUNREACHABLE;
	return distance;
#endif
}
#if 0

bool Intersect_R( const Tree& tree, int nodeID, const Float3& start, const Float3& end, float &t )
{
	// If < 0, we are in a leaf node
	if( nodeID < 0 ) {
		// return the minimum (closest) distance
		return nodeID == BSP_SOLID_LEAF;
	}

	const float epsilon = 1e-3f;

	const BspNode& node = tree.m_nodes[ nodeID ];
	const Vector4& plane = tree.m_planes[ node.plane ];
	// distance from plane for trace start and end
	const float d1 = Plane_PointDistance( plane, start );
	const float d2 = Plane_PointDistance( plane, end );
	// see which sides we need to consider
	if( d1 >= +epsilon && d2 >= +epsilon ) {
		nodeID = (INT16)node.front;
	}
	else if( d1 < -epsilon && d2 < -epsilon ) {
		nodeID = (INT16)node.back;
	}
	else {
		// Straddling

		if ( d1 < d2 ) {
			idist = 1.0f / (d1-d2);
			side = 1;
			frac2 = (d1 + offset) * idist;
			frac = (d1 - offset) * idist;
		} else if (d1 > d2) {
			idist = 1.0f / (d1-d2);
			side = 0;
			frac2 = (d1 - offset) * idist;
			frac = (d1 + offset) * idist;
		} else {
			side = 0;
			frac = 1.0f;
			frac2 = 0.0f;
		}

		// move up to the node
		if ( frac < 0.0f ) {
			frac = 0.0f;
		}
		else if ( frac > 1.0f ) {
			frac = 1.0f;
		}

		float fraction = ( d1 / ( d1 - d2 ) );
		mxASSERT( fraction >= 0.0f && fraction <= 1.0f );

		t = smallest( t, fraction );

		const Float3 mid = start + (end - start) * fraction;
		if(Intersect_R( tree, (INT16)node.front, start, mid, t )) {
			return true;
		}
		if(Intersect_R( tree, (INT16)node.back, mid, end, t )) {
			return true;
		}
		return false;
	}
	return false;
}
#endif

// This was copied straight from the book "Real-Time Collision Detection" by Christer Ericson (2005):
#if 0
// Intersect ray/segment R(t)=p+t*d, tmin <= t <= tmax, against bsp tree
// �node�, returning time thit of first intersection with a solid leaf, if any
int RayIntersect(BSPNode *node, Point p, Vector d, float tmin, float tmax, float *thit)
{
	std::stack<BSPNode *> nodeStack;
	std::stack<float> timeStack;
	assert(node != NULL);
	while (1) {
		if (!node->IsLeaf()) {
			float denom = Dot(node- >plane.n, d);
			float dist = node->plane.d - Dot(node->plane.n, p);
			int nearIndex = dist > 0.0f;
			// If denom is zero, ray runs parallel to plane. In this case,
			// just fall through to visit the near side (the one p lies on)
			if (denom != 0.0f) {
				float t = dist / denom;
				if (0.0f <= t && t <= tmax) {
					if (t >= tmin) {
						// Straddling, push far side onto stack, then visit near side
						nodeStack.push(node->child[1^nearIndex]);
						timeStack.push(tmax);
						tmax = t;
					} else nearIndex = 1^nearIndex;// 0 <= t < tmin, visit far side
				}
			}
			node = node->child[nearIndex];
		} else {
			// Now at a leaf. If it is solid, there�s a hit at time tmin, so exit
			if (node->IsSolid()) {
				*thit = tmin;
				return 1;
			}
			// Exit if no more subtrees to visit, else pop off a node and continue
			if (nodeStack.empty()) break;
			tmin = tmax;
			node = nodeStack.top(); nodeStack.pop();
			tmax = timeStack.top(); timeStack.pop();
		}
	}
	// No hit
	return 0;
}
#endif

void BspTree::Subtract( ATriangleMeshInterface* _mesh )
{
UNDONE;
}

namespace BSP
{

#if 0
static
void Dbg_ValidateNode_R( Tree & tree, UINT32 nodeIndex )
{
	const BspNode& node = tree.m_nodes[ nodeIndex ];

	if( node.IsInternal() )
	{
		mxASSERT( tree.m_planes.IsValidIndex( node.node.plane ) );
		mxASSERT( tree.m_nodes.IsValidIndex( node.front ) );
		mxASSERT( tree.m_nodes.IsValidIndex( node.back ) );
		
		Dbg_ValidateNode_R( tree, node.front );
		Dbg_ValidateNode_R( tree, node.back );
	}
}

static
void Dbg_DumpNode_R( Tree & tree, UINT32 nodeIndex, UINT32 depth = 0 )
{
	const BspNode& node = tree.m_nodes[ nodeIndex ];

	for(UINT32 i=0; i<depth; i++)
	{
		DBGOUT(" ");
	}
	if( node.IsInternal() )
	{
		DBGOUT("Inner node@%u: plane=%u, neg=%u, pos=%u\n"
			,nodeIndex,(UINT32)node.node.plane,(UINT32)node.back,(UINT32)node.front
			);

		Dbg_DumpNode_R( tree, node.front, depth+1 );
		Dbg_DumpNode_R( tree, node.back, depth+1 );
	}
	else
	{
		DBGOUT("%s leaf@%u\n",
			node.type==BN_Solid ? "Solid" : "Empty", nodeIndex);
	}
}
#endif

#if 0
struct pxPolygonCollector : pxTriangleIndexCallback
{
	PolygonList &	m_polygons;

	pxPolygonCollector( PolygonList & polygons )
		: m_polygons( polygons )
	{
	}
	virtual void ProcessTriangle( const Float3& p0, const Float3& p1, const Float3& p2 ) override
	{
		BspPoly & newPolygon = m_polygons.Add();

		// need to reverse winding (different culling in D3D11 renderer and id's winding)
#if 0
		newPolygon.AddPoint( p0 );
		newPolygon.AddPoint( p1 );
		newPolygon.AddPoint( p2 );
#else
		newPolygon.AddPoint( p2 );
		newPolygon.AddPoint( p1 );
		newPolygon.AddPoint( p0 );
#endif
	}
};
#endif

}//namespace BSP

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
