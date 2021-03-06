#pragma once

#include <Meshok/Meshok.h>
#include <Meshok/SDF.h>

typedef UINT16 BspNodeID;
typedef UINT16 BspPolyID;

enum { BSP_NONE = (UINT16)~0 };

struct BspNode : public CStruct
{
	UINT16		plane;	//�2 Hyperplane of the node (index into array of planes).
	BspNodeID	front;	//�2 Index of the right child (positive subspace, in front of plane).
	BspNodeID	back;	//�2 Index of the left child (negative subspace, behind the plane).
	BspPolyID	polys;	//�2 linked list of polygons lying on this node's plane
public:
	mxDECLARE_CLASS(BspNode,CStruct);
	mxDECLARE_REFLECTION;
};
struct BspVertex : public CStruct
{
	Float3	xyz;	//�12
	Half2	st;		//�4 16-bit floats in range [0..1]
public:
	mxDECLARE_CLASS(BspVertex,CStruct);
	mxDECLARE_REFLECTION;
};
struct BspPoly : public CStruct
{
	TArray< BspVertex >	vertices;
	BspPolyID			next;
	BspVertex			buffer[8];	//�128 small embedded storage to avoid memory allocations
public:
	mxDECLARE_CLASS(BspPoly,CStruct);
	mxDECLARE_REFLECTION;
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
struct BspTree : public CStruct
{
	TArray< BspNode >	m_nodes;	// tree nodes (0 = root index)
	TArray< Vector4 >	m_planes;	// plane equations (16 bytes per plane)
	TArray< BspPoly >	m_polys;
	//TArray< NodeData >	m_nodeData;
public:
	mxDECLARE_CLASS(BspTree,CStruct);
	mxDECLARE_REFLECTION;
	BspTree();

	struct Settings
	{
		//
	};

	ERet Build( ATriangleMeshInterface* triangleMesh );

	bool PointInSolid( const Float3& point, float epsilon ) const;

	// returns > 0, if outside
	float DistanceToPoint( const Float3& point, float epsilon ) const;

	float CastRay( const Float3& start, const Float3& direction ) const;

	bool Intersect( const Float3& start, const Float3& end, float &t ) const;

	size_t BytesAllocated() const;

	void Subtract( ATriangleMeshInterface* _mesh );

	void GenerateMesh();
};

// Special node ids.
enum {
	BSP_ROOT_NODE = 0,		// (0 = root index)
	// Negative numbers are leafs, not nodes
	// (-1) is reserved
	BSP_EMPTY_LEAF = -2,	// An outcell leaf node ( representing empty space ).
	BSP_SOLID_LEAF = -3,	// An incell leaf node ( representing solid space ).
};

namespace BSP
{

inline bool IsInternalNode( int nodeIndex )
{
	return nodeIndex > BSP_EMPTY_LEAF;
}

enum { BSP_MAX_NODES = MAX_UINT16-2 };	//-3 to account for BSP_SOLID_LEAF and BSP_EMPTY_LEAF
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
	FLOAT32	splitCost;

	// ratio balance of front/back polygons versus split polygons,
	// must be in range [0..1],
	// 1 - prefer balanced tree, 0 - avoid splitting polygons
	//
	FLOAT32	balanceVsCuts;

	// slack value for testing points wrt planes
	FLOAT32	planeEpsilon;

public:
	SplittingCriteria()
	{
		splitCost = 1.0f;
		balanceVsCuts = 0.6f;
		planeEpsilon = 0.017f;
	}
};

struct Volume : public AVolume
{
	TPtr< BspTree >	tree;

public:
	virtual float GetDistanceAt( const Float3& _position ) const override
	{
		return tree->DistanceToPoint( _position, 1e-4f );
	}
};


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
	BspNode
-----------------------------------------------------------------------------
*/
union BspNode
{
	LeafData	leaf;
	NodeData	node;

	// 8 bytes per each node
};
MX_DECLARE_POD_TYPE( BspNode );

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
	TList< BspNode >	m_nodes;	// tree nodes (0 = root index)
	TList< Plane3D >	m_planes;	// plane equations (16 bytes per plane)

public:
	BSP_Tree();

	bool PointInSolid( const Vec3D& point ) const;

	// negative distance - the point is inside solid region
	FLOAT32 DistanceToPoint( const Vec3D& point ) const;

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

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
