#pragma once
/*
some ideas on how to reduce memory consumption:
- store polygons in 2D (plane is known)
- don't store UVs per each vertex - store polygon's basis
- don't store polygons at all - derive b-rep (or, at least, some vertex attributes)
*/
#include <Base/Base.h>
#include <Core/Core.h>

#include <bgfx.h>

namespace BSP
{

struct Vertex
{
	Float3 xyz;
	UINT32 N;
	UINT32 T;
	Float2 UV;
	float c;	// unused padding
	//!32
public:
	Vertex()
	{}
	Vertex( const Float3& pos, UINT32 normal, UINT32 tangent, const Float2& texCoord )
		: xyz( pos ), N( normal ), T( tangent ), UV( texCoord )
	{}

	static bgfx::VertexDecl ms_decl;
	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::Tangent,   4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,    1, bgfx::AttribType::Float)
			.end();
	}
};
mxDECLARE_STRUCT(Vertex);
mxDECLARE_POD_TYPE(Vertex);

struct RayCastResult
{
	Float3	position;
	//Float3	normal;
	bool hitAnything;
public:
	RayCastResult()
	{
		hitAnything = false;
	}
	PREVENT_COPY(RayCastResult);
};

//
//	EPlaneSide - spatial relation to a plane.
//
enum EPlaneSide
{
	PLANESIDE_FRONT = 0,
	PLANESIDE_BACK,
	PLANESIDE_ON,
	PLANESIDE_CROSS,
};

//
// EPlaneType
//
enum EPlaneType
{
	PLANETYPE_X		= 0,
	PLANETYPE_Y,
	PLANETYPE_Z,
	PLANETYPE_NEGX,
	PLANETYPE_NEGY,
	PLANETYPE_NEGZ,
	PLANETYPE_TRUEAXIAL,	// all types < 6 are true axial planes
	PLANETYPE_ZEROX,
	PLANETYPE_ZEROY,
	PLANETYPE_ZEROZ,
	PLANETYPE_NONAXIAL,
};

struct ATriangleIndexCallback
{
	virtual void ProcessTriangle( const Vertex& a, const Vertex& b, const Vertex& c ) = 0;
	virtual ~ATriangleIndexCallback() {}
};

struct ATriangleMeshInterface : DbgNamedObject<>
{
	virtual void ProcessAllTriangles( ATriangleIndexCallback* callback ) = 0;
	virtual ~ATriangleMeshInterface() {}
};

template< class VERTEX, typename INDEX >
struct TProcessTriangles : public ATriangleMeshInterface
{
	const VERTEX* m_vertices;
	const int m_numVertices;
	const INDEX* m_indices;
	const int m_numIndices;
public:
	TProcessTriangles( const VERTEX* vertices, int numVertices, const INDEX* indices, int numIndices )
		: m_vertices( vertices ), m_numVertices( numVertices ), m_indices( indices ), m_numIndices( numIndices )
	{}
	virtual void ProcessAllTriangles( ATriangleIndexCallback* callback ) override
	{
		const int numTriangles = m_numIndices / 3;
		for( int i = 0; i < numTriangles; i++ )
		{
			const INDEX* tri = m_indices + i*3;
			callback->ProcessTriangle( m_vertices[tri[0]], m_vertices[tri[1]], m_vertices[tri[2]] );
			//callback->ProcessTriangle( m_vertices[tri[2]], m_vertices[tri[1]], m_vertices[tri[0]] );
		}
	}
};

enum NODE_TYPE
{
	INTERNAL_NODE = 0,
	SOLID_LEAF = 1,	// An incell leaf node ( representing solid matter ).
	EMPTY_LEAF = 2,	// An outcell leaf node ( representing empty space ).
};

typedef UINT16 NodeID;	//� upper two bits describe the type of the node
typedef UINT16 FaceID;
typedef UINT16 PlaneID;


inline bool IS_LEAF( NodeID nodeID ) {
	return (nodeID & (~0U<<14)) != 0;
}
inline bool IS_INTERNAL( NodeID nodeID ) {
	return (nodeID & (~0U<<14)) == 0;
}
inline NodeID MAKE_LEAF( NODE_TYPE type ) {
	return type<<14;
}
inline NODE_TYPE GET_TYPE( NodeID nodeID ) {
	return NODE_TYPE((nodeID >> 14) & 3);
}
inline UINT16 GET_PAYLOAD( NodeID nodeID ) {
	return nodeID & ~(~0U<<14);
}
inline bool IS_SOLID_LEAF( NodeID nodeID ) {
	return GET_TYPE(nodeID) == SOLID_LEAF;
}
inline bool IS_EMPTY_LEAF( NodeID nodeID ) {
	return GET_TYPE(nodeID) == EMPTY_LEAF;
}



enum { NIL_INDEX = (UINT16)~0 };

//NOTE: lower 14 bits of a leaf index may contain additional information (e.g. material ID)
struct Node : public CStruct
{
	UINT16	plane;	//�2 Hyperplane of the node (index into array of planes).
	NodeID	front;	//�2 Index of the right child (positive subspace, in front of plane).
	NodeID	back;	//�2 Index of the left child (negative subspace, behind the plane).
	FaceID	faces;	//�2 linked list of polygons lying on this node's plane
public:
	mxDECLARE_CLASS(Node,CStruct);
	mxDECLARE_REFLECTION;
	//Node();
};
struct Face : public CStruct
{
	//TArray< Vertex >	vertices;	//�8/12
TStaticList< Vertex,36 >	vertices;	//�8/12
	FaceID				next;		//�2
	Vertex				buffer[7];	//�224 small embedded storage to avoid memory allocations
	//�240/244
public:
	mxDECLARE_CLASS(Face,CStruct);
	mxDECLARE_REFLECTION;
	Face();
};

/*
-----------------------------------------------------------------------------
	stats for testing & debugging
-----------------------------------------------------------------------------
*/
class BspStats {
public:
	UINT32		m_polysBefore;
	UINT32		m_polysAfter;	// number of resulting polygons
	UINT32		m_numSplits;	// number of cuts caused by BSP

	UINT32		m_numInternalNodes;
	UINT32		m_numPlanes;
	//UINT32		depth;
	UINT32		m_numSolidLeaves, m_numEmptyLeaves;

	UINT32		m_bytesAllocated;
public:
	BspStats();
	void Reset();
	void Print( UINT32 elapsedTimeMSec );
};

/*
-----------------------------------------------------------------------------
	Polygon-aligned solid leaf-labeled BSP tree.

	used mainly for collision detection.

	@todo:
	maybe, make the tree absolutely pointerless (it can only be done if it's static,
	e.g. calculate child indices as 2n and 2n+1, where n - parent index).

	also, could save a lot of memory if leaves were implicit, i.e.
	use only 4 types of BSP nodes:
	(in,plane,out), (in,plane,plane), (plane,plane,out), (plane,plane,plane).
	but that would complicate the resulting code
	(and wouldn'tt allow storing any info in leaf nodes).
-----------------------------------------------------------------------------
*/
struct Tree : public CStruct
{
	TArray< Vector4 >	m_planes;	// plane equations (16 bytes per plane)
	TArray< Node >		m_nodes;	// tree nodes (0 = root index)
	TArray< Face >		m_faces;	// convex polygons
	//TArray< NodeData >	m_nodeData;
public:
	mxDECLARE_CLASS(Tree,CStruct);
	mxDECLARE_REFLECTION;
	Tree();

	struct Settings
	{
		//
	};

	ERet Build( ATriangleMeshInterface* triangleMesh );

	bool PointInSolid( const Float3& point, float epsilon ) const;

	// returns > 0, if outside
	float DistanceToPoint( const Float3& point, float epsilon ) const;


	void CastRay(
		const Float3& start, const Float3& direction,
		RayCastResult &result
	) const;

	bool CastRay(
		const Float3& start, const Float3& direction,
		float tmin, float tmax, float *thit
	) const;

	float CastRay( const Float3& start, const Float3& direction ) const;

	bool Intersect( const Float3& start, const Float3& end, float &t ) const;

	size_t BytesAllocated() const;

	void CopyFrom( const Tree& other );

	void Subtract( Tree& other );
	void Subtract2( Tree& other, const Tree& temp );

	void Negate();
	void Translate( const Float3& T );

	void GenerateMesh(
		TArray< Vertex > &vertices,
		TArray< UINT16 > &indices,
		const NodeID start = 0
	) const;

public:	// Internal functions:

	int PartitionPolygons(
		const Vector4& partitioner,
		const FaceID polygons,	// linked list
		FaceID *frontFaces,	// linked list
		FaceID *backFaces,	// linked list
		FaceID *coplanar,	// linked list
		int faceCounts[4],	// EPlaneSide
		const float epsilon = 0.13f
	);

	EPlaneSide PartitionNodeWithPlane(
		const Vector4& partitioner,
		const NodeID nodeId,
		NodeID *front,
		NodeID *back
	);

#if 0
	int FindPlaneIndex(
		const Vector4& plane,
		const float normal_epsilon,
		const float distance_epsilon
	) const;

	UINT16 AddUniquePlane(
		const Vector4& plane
	);
#endif

	NodeID NewNode();
	FaceID AddPolygon( const Vertex* points, const int numPoints, FaceID * head );
};

enum { BSP_MAX_NODES = (1U<<14)-1 };
enum { BSP_MAX_DEPTH = 32 };	// size of temporary stack storage (we try to avoid recursion)
enum { BSP_MAX_PLANES = MAX_UINT16-1 };	// maximum allowed number of planes in a single tree
enum { BSP_MAX_POLYS = MAX_UINT16-1 };

/*
-----------------------------------------------------------------------------
	SplittingCriteria
-----------------------------------------------------------------------------
*/
struct SplittingCriteria
{
	float	splitCost;

	// ratio balance of front/back polygons versus split polygons,
	// must be in range [0..1],
	// 1 - prefer balanced tree, 0 - avoid splitting polygons
	//
	float	balanceVsCuts;

	// slack value for testing points wrt planes
	float	planeEpsilon;

public:
	SplittingCriteria()
	{
		splitCost = 1.0f;
		balanceVsCuts = 0.6f;
		planeEpsilon = 0.017f;
	}
};

void TriangulateFace(
					  const Face& face,
					  TArray< Vertex > &vertices,
					  TArray< UINT16 > &indices
					  );

void TriangulateFaces(
					  const Tree& tree,
					  const FaceID faces,
					  TArray< Vertex > &vertices,
					  TArray< UINT16 > &indices
					  );

NodeID CopySubTree(
				   Tree & treeA,
				   const Tree& treeB, const NodeID iNodeB
				   );

FaceID ClipFacesOutsideBrush(
								  Tree & treeA, const FaceID facesA,
								  const Tree& treeB, const NodeID iNodeB
								 );

EPlaneSide SplitConvexPolygonByPlane(
	const Vertex* vertices,
	const int vertexCount,
	TArray<Vertex> &front,	// valid only if the polygon was split
	TArray<Vertex> &back,	// valid only if the polygon was split
	const Vector4& plane,
	const float epsilon
	);

namespace Debug
{
	void PrintTree( const Tree& tree, const NodeID start = 0 );
	int CalculateFaceCount( const Tree& tree, const FaceID faces );
	void PrintFaceList( const Tree& tree, const FaceID faces );
}//namespace Debug

#if 0
namespace BSP
{
	// This enum describes the allowed BSP node types.
	enum ENodeType
	{
		BN_Polys,	// An internal (auto-)partitioning node (with polygon-aligned splitting plane).
		BN_Solid,	// An incell leaf node ( representing solid space ).
		BN_Empty,	// An outcell leaf node ( representing empty space ).
		BN_Split,	// An internal partitioning node (with arbitrary splitting plane).
		// Used by progressive BSP trees.
		BN_Undecided,
		//// For axis-aligned splitting planes.
		//BN_Plane_YZ,	// X axis
		//BN_Plane_XY,	// Z axis
		//BN_Plane_XZ,	// Y axis

		BN_MAX	// Marker. Do not use.
	};

	//
	//	E_TraversalType
	//
	enum TraversalType
	{
		Traverse_PreOrder,	// Visit the root. Traverse the left subtree. Traverse the right subtree.
		Traverse_PostOrder,	// Traverse the left subtree. Traverse the right subtree. Visit the root.
		Traverse_InOrder,	// Traverse the left subtree. Visit the root. Traverse the right subtree.
	};

	// generic BSP tree used as intermediate representation
	// for constructing specific optimized trees
	//
	struct genNode
	{
		TPtr< genNode >	pos;
		TPtr< genNode >	neg;
	};

	struct genTree
	{
		TPtr< genNode >	root;

		void Build( pxTriangleMeshInterface* mesh );
	};

	//
	//	ENodeState
	//
	enum ENodeState
	{
		Unchanged,	// Node has not been modified.
		LeftChild,	// The left child has not been modified.
		RightChild,	// The right child has not been modified.
	};

}//namespace BSP





struct LeafData
{
	U2	poly;	//�2 Index of the first polygon in a linked list.
	U2	flags;
	U2	userData;	// e.g. material index
	U2	_padTo32;	// reserved for future use
};
struct NodeData
{
	U2	plane;	//�2 Hyperplane of the node (index into array of planes).

	union
	{
		struct
		{
			U2	pos;	//�2 Index of the right child (positive subspace, in front of plane).
			U2	neg;	//�2 Index of the left child (negative subspace, behind the plane).
		};
		U2	kids[2];
	};

	U2	_padTo32;	// reserved for future use
};



// NOTE: 0 can also be used to mark empty leaf nodes (representing convex regions of space).
// there should be no ambiguity, because only the root (internal) node can be accessed with zero index.
//enum { BSP_EMPTY_LEAF = 0 };
enum { BSP_EMPTY_LEAF = (U2)-2 };	// An outcell leaf node ( representing empty space ).
enum { BSP_SOLID_LEAF = (U2)-3 };	// An incell leaf node ( representing solid space ).

FORCEINLINE bool IsLeafNodeId( UINT nodeNum )
{return nodeNum == BSP_EMPTY_LEAF || nodeNum == BSP_SOLID_LEAF;}


/*
-----------------------------------------------------------------------------
	Node
-----------------------------------------------------------------------------
*/
union Node
{
	LeafData	leaf;
	NodeData	node;

	// 8 bytes per each node
};
MX_DECLARE_POD_TYPE( Node );

/*
-----------------------------------------------------------------------------
	BSP_Tree

	the tree is static,
	it's assumed that all planes are already in world space,
	and the corresponding physics object's transform is identity
-----------------------------------------------------------------------------
*/
MX_ALIGN_16(struct) BSP_Tree
{
	TList< Node >	m_nodes;	// tree nodes (0 = root index)
	TList< Plane3D >	m_planes;	// plane equations (16 bytes per plane)

public:
	BSP_Tree();

	bool PointInSolid( const Vec3D& point ) const;

	// negative distance - the point is inside solid region
	float DistanceToPoint( const Vec3D& point ) const;

	// overlap testing

	bool TestOverlapConvex(
		const pxShape_Convex* convexShape,
		const pxTransform& shapeTransform,	// convex local to world transform
		pxContactManifold & manifold
		) const;


	// sweep tests

	void TraceAABB(
		const AABB& boxsize, const Vec3D& start, const Vec3D& end,
		FLOAT & fraction, Vec3D & normal
		) const;

	// oriented capsule collision detection
	//




	pxVec3 CalcSupportingVertex( const pxVec3& dir ) const;

	// Returns the total amount of occupied memory in bytes.
	size_t GetMemoryUsed() const;

	void Serialize( mxArchive& archive );

public:
#if MX_EDITOR
	SBspStats	m_stats;

	void Build( pxTriangleMeshInterface* triangleMesh );
#endif // MX_EDITOR
};





enum LPProblemStatus
{
	Redundant,
	Proper
};

/*
=======================================================================
	
		Constructive Solid Geometry.

=======================================================================
*/

//
//	ESetOp - enumerates all supported types of CSG operations.
//
enum ESetOp
{
	CSG_Difference,		// 'subtract'
	CSG_Union,			// 'add'
//	CSG_Intersection	// 'and', find the common part
};

const char* ESetOp_To_Chars( ESetOp op );

//
//	CSGInput - contains settings for performing boolean set operations.
//
//struct CSGInput
//{
//	ESetOp			type;		// type of CSG operation to perform
//	CSGModel *		operand;	// the second operand
//};
#endif

}//namespace BSP
