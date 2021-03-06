#pragma once
#include <RPagedBuffer.h>
#include <tuple>
#include "vertextypes.h"
#include "visualStorage.h"

namespace Engine
{
	class GameEngine;
}

namespace ZenConvert
{
	struct PackedMesh;
}

namespace Renderer
{
	class Visual;
	class SkeletalMeshVisual;
	class StaticMeshVisual;
	class RenderSystem
	{
	public:
		RenderSystem(Engine::GameEngine& engine);
		~RenderSystem();

		/**
		 * @brief Renders a frame
		 */
		void renderFrame(float deltaTime);

		/**
		 * @brief Returns a paged buffer of the given type, if one was created.
		 *		  This only works if the type you ask for was defined in the arguments 
		 *		  of m_PagedBuffers.
		 */
		template <typename T>
		RAPI::RPagedBuffer<T>& getPagedVertexBuffer(){return *std::get<RAPI::RPagedBuffer<T>*>(m_PagedVertexBuffers.types);}

		template <typename T>
		RAPI::RPagedBuffer<T>& getPagedIndexBuffer(){return *std::get<RAPI::RPagedBuffer<T>*>(m_PagedIndexBuffers.types);}

		/**
		 * @brief returns the engine this was created with
		 */
		Engine::GameEngine* getEngine(){return m_pEngine;}

		/**
		 * @brief Creates a visual for the given type in the target function overload
		 */
		StaticMeshVisual* createVisual(size_t hash, const ZenConvert::PackedMesh& packedMesh);
		SkeletalMeshVisual* createVisual(size_t hash, const ZenConvert::PackedSkeletalMesh& packedMesh);

		/**
		 * @brief Returns the visual matching the given hash
		 */
		Visual* getVisualByHash(size_t hash);

		/**
		 * @brief clears the instance cache vector
		 */
		void clearInstanceCache();

		/**
		 * @brief Adds an entity for the given visual
		 * @return current number of instances in the vector
		 */
		size_t addEntityForVisual(Engine::ObjectHandle entity, size_t visualId, size_t subVisualId);

		/**
		 * @brief Builds instancing-data and stores it into the given RBuffer
		 *		  and modifies the pipelinestate of the main-entities
		 */
		void buildInstancingData(RAPI::RBuffer* targetBuffer, std::vector<Engine::ObjectHandle>& mainHandles);

		/** 
		 * @brief returns the global visual storage
		 */
		VisualStorage& getVisualStorage() { return m_VisualStorage;}

		RAPI::RBuffer* getInstancingBuffer(){ return m_pInstancingBuffer; }
	protected:

		/**
		* @brief instancing cache
		*/
		std::vector<PerInstanceData> m_InstanceCache;
		std::vector<size_t> m_MaxEntitiesForVisual;

		template<RAPI::EBindFlags B>
		struct PagedBuffer
		{
			template<typename... T>
			struct PBTuple
			{
				PBTuple() : types((new RAPI::RPagedBuffer<T>(B))...){}
				std::tuple<RAPI::RPagedBuffer<T>*...> types;
			};
		};

		/**
		 * @brief Paged buffers for the given types. These devide a single large physical buffer
		 *		  into smaller logical buffers, helping circumvent switching the buffers many 
		 *		  times while rendering
		 */
		PagedBuffer<RAPI::B_VERTEXBUFFER>::PBTuple<WorldVertex, SkeletalVertex> m_PagedVertexBuffers;
		PagedBuffer<RAPI::B_INDEXBUFFER>::PBTuple<uint32_t> m_PagedIndexBuffers;

		/**
		 * @brief Engine this was created with
		 */
		Engine::GameEngine* m_pEngine;

		/**
		 * @brief cache for loaded visuals
		 */
		VisualStorage m_VisualStorage;

		/**
		 * @brief Instancing-cache for each visual. m_VisualInstanceCache stores VisualIDs, while an InstanceCacheEntry stores the subIds
		 */
		struct InstanceCacheEntry
		{
			Engine::ObjectHandle mainHandle;
			std::vector<PerInstanceData> instanceDataPerSubIdx;
		};
		std::vector<std::vector<InstanceCacheEntry>> m_VisualInstanceCache;
		size_t m_NumRegisteredInstances;
		RAPI::RBuffer* m_pInstancingBuffer;
	};
}