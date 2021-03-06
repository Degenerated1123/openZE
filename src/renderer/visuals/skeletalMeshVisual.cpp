#include "SkeletalMeshVisual.h"
#include "../renderSystem.h"
#include <REngine.h>
#include <RResourceCache.h>
#include <RTexture.h>
#include <RTools.h>
#include <RStateMachine.h>
#include <RDevice.h>
#include <RBuffer.h>
#include "../visualLoad.h"
#include "engine/game/gameengine.h"
#include "engine/components/visual.h"

using namespace Renderer;

// TODO: This file is the same as the StaticMeshVisual with only minor differences. Merge these somehow!


Renderer::SkeletalMeshVisual::SkeletalMeshVisual(RenderSystem& system, Engine::ObjectFactory& factory) : Visual(system, factory)
{
}

SkeletalMeshVisual::~SkeletalMeshVisual()
{
}

/**
 * @brief Initializes the mesh with the given dataset
 */
void Renderer::SkeletalMeshVisual::createMesh(const ZenConvert::PackedSkeletalMesh& packedMesh)
{
	m_Submeshes.resize(packedMesh.subMeshes.size());

	// Copy vertex data to the paged buffer
	m_pVertexBuffer = m_pRenderSystem->getPagedVertexBuffer<SkeletalVertex>().AddLogicalBuffer(reinterpret_cast<const SkeletalVertex*>(packedMesh.vertices.data()), packedMesh.vertices.size());

	// Copy material info and indices for each submesh
	for(size_t i = 0, end = packedMesh.subMeshes.size(); i < end; i++)
	{
		auto& source = packedMesh.subMeshes[i];
		auto& target = m_Submeshes[i];

		// Material
		target.material = source.material;

		// Indices
		target.indexBuffer = m_pRenderSystem->getPagedIndexBuffer<uint32_t>().AddLogicalBuffer(source.indices.data(), source.indices.size());
	}

	// Register observers, so we can update our pipeline-states accordingly
	m_pRenderSystem->getPagedVertexBuffer<SkeletalVertex>().RegisterObserver(this, [this](unsigned int id, void* userptr) { onLogicalVertexBuffersUpdated(userptr); } );
	m_pRenderSystem->getPagedIndexBuffer<uint32_t>().RegisterObserver(this, [this](unsigned int id, void* userptr) { onLogicalIndexBuffersUpdated(userptr); } );
}

/**
* @brief Updates the created pipelinestates accordingly to the logical buffers
*/
void Renderer::SkeletalMeshVisual::onLogicalIndexBuffersUpdated(void* userData)
{
	// The paged buffers put their final data into the userData
	uint32_t* indices = (unsigned int*)userData;

	for(size_t i = 0, end = m_Submeshes.size(); i < end; i++)
	{
		// Modify the data to match the index offsets
		for(unsigned int j = 0; j < m_Submeshes[i].indexBuffer->PageNumElements; j++)
		{
			// Add the page-offset of the vertexbuffer so the indices match the vertices
			indices[m_Submeshes[i].indexBuffer->PageStart + j] += m_pVertexBuffer->PageStart;
		}

		// Modify instance index
		for(auto& h : m_Submeshes[i].submeshObjectHandles)
		{
			Engine::Components::Visual* pVisual = m_pObjectFactory->storage().getComponent<Engine::Components::Visual>(h);

			if(!pVisual)
			{
				LogWarn() << "StaticMeshVisual has reference to invalid visual entity!";
				continue;
			}

			pVisual->pPipelineState->StartIndexOffset = m_Submeshes[i].indexBuffer->PageStart;
		}
	}
}

void Renderer::SkeletalMeshVisual::onLogicalVertexBuffersUpdated(void* userptr)
{
	(void)userptr;
}

/**
* @brief Creates entities matching this visual
*/
void Renderer::SkeletalMeshVisual::createEntities(std::vector<Engine::ObjectHandle>& createdEntities)
{
	// Create buffers and states for each texture
	RAPI::RPixelShader* ps = RAPI::REngine::ResourceCache->GetCachedObject<RAPI::RPixelShader>("simplePS");
	RAPI::RVertexShader* vs = RAPI::REngine::ResourceCache->GetCachedObject<RAPI::RVertexShader>("skeletalVS");
	RAPI::RStateMachine& sm = RAPI::REngine::RenderingDevice->GetStateMachine();

	RAPI::RInputLayout* inputLayout = RAPI::RTools::CreateInputLayoutFor<Renderer::SkeletalVertex>(vs);
	RAPI::RSamplerState* ss;
	RAPI::RRasterizerState* rs;
	RAPI::RTools::MakeDefaultStates(nullptr, &ss, nullptr, &rs);

	// Setup default states
	RAPI::RSamplerStateInfo ssi;
	ssi.SetDefault();
	ssi.MaxAnisotropy = 16.0f;
	ssi.Filter = RAPI::FILTER_ANISOTROPIC;
	ss = RAPI::RTools::GetState(ssi);

	sm.SetSamplerState(ss);
	sm.SetRasterizerState(rs);
	sm.SetPixelShader(ps);
	sm.SetVertexShader(vs);
	sm.SetInputLayout(inputLayout);

	// Create object buffer
	RAPI::RBuffer* pObjectBuffer = RAPI::REngine::ResourceCache->CreateResource<RAPI::RBuffer>();
	sm.SetConstantBuffer(1, pObjectBuffer, RAPI::ST_VERTEX);
	sm.SetConstantBuffer(0, RAPI::REngine::ResourceCache->GetCachedObject<RAPI::RBuffer>("PerFrameCB"), RAPI::ST_VERTEX);

	// Initialize object-buffer
	Math::Matrix m = Math::Matrix::CreateIdentity();
	ZenConvert::SkeletalMeshInstanceInfo ocb;
	ocb.worldMatrix = m;
	ocb.color = Math::float4(1,1,1,1);
	pObjectBuffer->Init(&ocb, sizeof(ZenConvert::SkeletalMeshInstanceInfo), sizeof(ZenConvert::SkeletalMeshInstanceInfo), RAPI::EBindFlags::B_CONSTANTBUFFER, RAPI::U_DYNAMIC, RAPI::CA_WRITE);

	// Create an entity for each submesh
	createdEntities.reserve(m_Submeshes.size());
	size_t i=0;
	for(auto& s : m_Submeshes)
	{
		sm.SetVertexBuffer(0, m_pRenderSystem->getPagedVertexBuffer<SkeletalVertex>().GetBuffer());
		sm.SetIndexBuffer(m_pRenderSystem->getPagedIndexBuffer<uint32_t>().GetBuffer());
		sm.SetTexture(0, loadTexture(s.material.texture, m_pRenderSystem->getEngine()->vdfsFileIndex()), RAPI::ST_PIXEL);

		// Make entity
		Engine::ObjectHandle e = m_pObjectFactory->storage().createEntity();
		Engine::Entity* entity = m_pObjectFactory->storage().getEntity(e);
		m_pObjectFactory->storage().addComponent<Engine::Components::Visual>(e);

		// Create visual
		Engine::Components::Visual* visual = m_pObjectFactory->storage().getComponent<Engine::Components::Visual>(e);

		if(visual)
		{
			visual->pInstanceBuffer = nullptr;
			visual->pObjectBuffer = pObjectBuffer;
			visual->colorMod = Math::float4(1.0f, 1.0f, 1.0f, 1.0f);

			visual->pPipelineState = sm.MakeDrawCallIndexed(s.indexBuffer->PageNumElements, s.indexBuffer->PageStart);

			entity->setWorldTransform(Math::Matrix::CreateIdentity());
			visual->visualId = m_Id;
			visual->visualSubId = i;
		}

		createdEntities.push_back(e);
		m_ObjectHandles.insert(e);
		s.submeshObjectHandles.insert(e);

		i++;
	}

	m_NumEntities++;
}