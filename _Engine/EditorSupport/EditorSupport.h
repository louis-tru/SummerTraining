#pragma once

#if MX_AUTOLINK
#pragma comment( lib, "EditorSupport.lib" )
#endif //MX_AUTOLINK

#include <Core/Asset.h>

namespace EditorUtil
{
	//-----------------------------------------------------------------------
	// Reference tracking
	//-----------------------------------------------------------------------

	struct ObjectInfo
	{
		CStruct *		o;
		const mxClass *	type;
		UINT			numReferences;
	};
	struct ObjectReference
	{
		CStruct *		o;
		const mxClass *	type;
		void *			pointer;	// we may want to patch this pointer (e.g. upon object deletion)

	public:
		ObjectReference()
		{
			o = nil;
			type = nil;
			pointer = nil;
		}
		ObjectReference( const ObjectReference& other )
		{
			o = other.o;
			type = other.type;
			pointer = other.pointer;
		}
		bool operator == ( const ObjectReference& other ) const
		{
			return o == other.o
				&& type == other.type
				&& pointer == other.pointer
				;
		}
		bool operator != ( const ObjectReference& other ) const
		{
			return !(*this == other);
		}
		bool DbgCheckValid() const
		{
			chkRET_FALSE_IF_NIL(o);
			chkRET_FALSE_IF_NIL(type);
			chkRET_FALSE_IF_NIL(pointer);
			return true;
		}
	};

	typedef TArray< ObjectReference >	ListOfReferences;

	void DBG_PrintReferences( const ListOfReferences& references );

	UINT CollectPointersFromObject( CStruct* o, const mxClass& type, ListOfReferences &references );

	UINT GetNumReferencesToObject( CStruct* o, const Clump& clump );
	UINT CollectReferencesToObject( CStruct* o, const Clump& clump, ListOfReferences &references );

	void PatchReferences( ListOfReferences & references, void* newValue );

	bool TryDeleteAndCleanupGarbage( CStruct* o, const mxClass& type, Clump & clump );

	void DeleteUnreferencedObjects( Clump& clump );

	//-----------------------------------------------------------------------
	// Asset management
	//-----------------------------------------------------------------------

	//struct AssetTypeInfo
	//{
	//	String	className;		// name of C++ class representing the asset (e.g. rxMesh, rxTexture)
	//	String	assetType;		// name of asset type (e.g. Mesh, Texture)
	//	String	fileExtension;	// exported asset file extension with dot, e.g. '.mesh', '.texture'
	//};
	//void GetListOfAssetTypeInfo( AssetTypeInfo &output );

}//namespace EditorUtil

//namespace AssetPipeline
//{
//	extern const Chars	ASSET_TYPE_MESH;
//	extern const Chars	ASSET_TYPE_TEXTURE;
//	extern const Chars	ASSET_TYPE_MATERIAL;
//	extern const Chars	ASSET_TYPE_SCRIPT;
//	extern const Chars	ASSET_TYPE_SHADER;
//	extern const Chars	ASSET_TYPE_PHYSICS_MODEL;
//	extern const Chars	ASSET_TYPE_COLLISION_HULL;
//	extern const Chars	ASSET_TYPE_MESH_ANIMATION;
//	extern const Chars	ASSET_TYPE_WAY_POINT_DEF;
//	extern const Chars	ASSET_TYPE_LEVEL_CHUNK;
//
//}//namespace AssetPipeline

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
