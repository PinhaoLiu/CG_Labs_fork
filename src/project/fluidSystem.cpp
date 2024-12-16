#include "fluidSystem.hpp"

FluidSystem::FluidSystem()
{
	m_unitScale = 0.004f;
	m_viscosity = 1.0f;
	m_restDensity = 1000.0f;
	m_pointMass = 0.0006f;
	m_gasConstantK = 1.0f;
	m_smoothRadius = 0.01f;
	m_pointDistance = 0.0f;

	m_rexSize[0] = 0;
	m_rexSize[1] = 0;
	m_rexSize[2] = 0;

	m_boundaryStiffness = 50.0f;
	m_boundaryDampening = 256.0f;
	m_speedLimiting = 200.0f;

	m_kernelPoly6 = 315.0f / (64.0f * glm::pi<float>() * glm::pow(m_smoothRadius, 9.0f));
	m_kernelSpiky = -45.0f / (glm::pi<float>() * glm::pow(m_smoothRadius, 6.0f));
	m_kernelViscosity = 45.0f / (glm::pi<float>() * glm::pow(m_smoothRadius, 6.0f));
}

void FluidSystem::tick()
{
	m_gridContainer.insertParticles(&m_pointBuffer);
	_computePressure();
	_computeForce();
	_advance();
}

FluidSystem::~FluidSystem() {}

void FluidSystem::_init(ushort maxPointCounts, const fBox3& wallBox, const fBox3& initFluidBox, const v3& gravity)
{
	m_pointBuffer.reset(maxPointCounts);
	m_sphWallBox = wallBox;
	m_gravityDir = gravity;
	m_pointDistance = glm::pow(m_pointMass / m_restDensity, 1.0f / 3.0f);
	_addFluidVolume(initFluidBox, m_pointDistance / m_unitScale);
	m_gridContainer.init(wallBox, m_unitScale, m_smoothRadius * 2.0f, 1.0f, m_rexSize);
	posData = std::vector<float>(3.0f * m_pointBuffer.size(), 0);
}

void FluidSystem::_computePressure()
{
	float h2 = m_smoothRadius * m_smoothRadius;
	m_neighborTable.reset(m_pointBuffer.size());

	for (uint i = 0; i < m_pointBuffer.size(); i++) {
		Point* pi = m_pointBuffer.get(i);
		float sum = 0.0f;
		m_neighborTable.point_prepare(i);
		int gridCell[8];
		m_gridContainer.findCells(pi->pos, m_smoothRadius / m_unitScale, gridCell);

		for (int cell = 0; cell < 8; cell++) {
			if (gridCell[cell] == -1)
				continue;

			int pndx = m_gridContainer.getGridData(gridCell[cell]);
			while (pndx != -1)
			{
				Point* pj = m_pointBuffer.get(pndx);
				if (pj == pi) {
					sum += glm::pow(h2, 3.0f);
				}
				else {
					v3 pi_pj = (pi->pos - pj->pos) * m_unitScale;
					float r2 = pi_pj.x * pi_pj.x + pi_pj.y * pi_pj.y + pi_pj.z * pi_pj.z;
					if (h2 > r2) {
						float h2_r2 = h2 - r2;
						sum += glm::pow(h2_r2, 3.0f);
						if (!m_neighborTable.point_add_neighbor(pndx, glm::sqrt(r2)))
							goto NEIGHBOR_FULL;
					}
				}
				pndx = pj->next;
			}
		} 
	NEIGHBOR_FULL:
		m_neighborTable.point_commit();
		pi->density = m_kernelPoly6 * m_pointMass * sum;
		pi->pressure = (pi->density - m_restDensity) * m_gasConstantK;
	}
}

void FluidSystem::_computeForce()
{
	float h2 = m_smoothRadius * m_smoothRadius;

	for (uint i = 0; i < m_pointBuffer.size(); i++) {
		Point* pi = m_pointBuffer.get(i);
		v3 accel_sum = v3(0.0f);
		int neighborCounts = m_neighborTable.getNeighborCounts(i);

		for (uint j = 0; j < neighborCounts; j++) {
			ushort neighborIndex;
			float r;
			m_neighborTable.getNeighborInfo(i, j, neighborIndex, r);
			Point* pj = m_pointBuffer.get(neighborIndex);

			v3 ri_rj = (pi->pos - pj->pos) * m_unitScale;
			float h_r = m_smoothRadius - r;
			float pterm = -m_pointMass * m_kernelSpiky * h_r * h_r *
				(pi->pressure + pj->pressure) / (2.0f * pi->density * pj->density);
			accel_sum += ri_rj * pterm / r;

			float vterm = m_kernelViscosity * m_viscosity * h_r *
				m_pointMass / (pi->density * pj->density);

			accel_sum += (pj->velocity_eval - pi->velocity_eval) * vterm;
		}

		pi->accel = accel_sum;
	}
}

void FluidSystem::_advance()
{
	float deltaTime = 0.003f;
	float SL2 = m_speedLimiting * m_speedLimiting;

	for (uint i = 0; i < m_pointBuffer.size(); i++) {
		Point* p = m_pointBuffer.get(i);
		v3 accel = p->accel;
		float accel2 = accel.x * accel.x + accel.y * accel.y + accel.z * accel.z;

		if (accel2 > SL2)
			accel *= m_speedLimiting / glm::sqrt(accel2);

		float diff;

		diff = 1 * m_unitScale - (p->pos.z - m_sphWallBox.min.z) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(0.0f, 0.0f, 1.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.z = glm::abs(p->velocity.z);
			p->velocity_eval.z = glm::abs(p->velocity_eval.z);
		}

		diff = 1 * m_unitScale - (m_sphWallBox.max.z - p->pos.z) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(0.0f, 0.0f, -1.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.z = -glm::abs(p->velocity.z);
			p->velocity_eval.z = -glm::abs(p->velocity_eval.z);
		}

		diff = 1 * m_unitScale - (p->pos.x - m_sphWallBox.min.x) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(1.0f, 0.0f, 0.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.x = glm::abs(p->velocity.x);
			p->velocity_eval.x = glm::abs(p->velocity_eval.x);
		}

		diff = 1 * m_unitScale - (m_sphWallBox.max.x - p->pos.x) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(-1.0f, 0.0f, 0.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.x = -glm::abs(p->velocity.x);
			p->velocity_eval.x = -glm::abs(p->velocity_eval.x);
		}

		diff = 1 * m_unitScale - (p->pos.y - m_sphWallBox.min.y) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(0.0f, 1.0f, 0.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.y = glm::abs(p->velocity.y);
			p->velocity_eval.y = glm::abs(p->velocity_eval.y);
		}

		diff = 1 * m_unitScale - (m_sphWallBox.max.y - p->pos.y) * m_unitScale;
		if (diff > 0.0f) {
			v3 norm(0.0f, -1.0f, 0.0f);
			float adj = m_boundaryStiffness * diff - m_boundaryStiffness * glm::dot(norm, p->velocity_eval);
			accel += adj * norm;
			p->velocity.y = -glm::abs(p->velocity.y);
			p->velocity_eval.y = -glm::abs(p->velocity_eval.y);
		}

		accel += m_gravityDir;

		v3 vnext = p->velocity + accel * deltaTime;
		p->velocity_eval = (p->velocity + vnext) * 0.5f;
		p->velocity = vnext;
		p->pos += vnext * deltaTime / m_unitScale;

		posData[3 * i] = p->pos.x;
		posData[3 * i + 1] = p->pos.y;
		posData[3 * i + 2] = p->pos.z;
	}
}

void FluidSystem::_addFluidVolume(const fBox3& fluidBox, float spacing)
{
	for (float z = fluidBox.max.z; z >= fluidBox.min.z; z -= spacing) {
		for (float y = fluidBox.max.y; y >= fluidBox.min.y; y -= spacing) {
			for (float x = fluidBox.max.x; x >= fluidBox.min.x; x -= spacing) {
				Point* p = m_pointBuffer.addPointReuse();
				p->pos = v3(x, y, z);
			}
		}
	}
}
