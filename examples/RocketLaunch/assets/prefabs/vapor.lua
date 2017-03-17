function buildVaporFX( transformation )
	
	local MAX_PARTICLES = 1000
	
	return {
		type = 'crimild::Group',
		components = {
			{
				type = 'crimild::ParticleSystemComponent',
				maxParticles = MAX_PARTICLES,
				emitRate = 0.1 * MAX_PARTICLES,
				computeInWorldSpace = true,
				generators = {
					{
						type = 'crimild::BoxPositionParticleGenerator',
						origin = { 0.0, 0.0, 0.0 },
						size = { 1.0, 0.0, 1.0 },
					},
					{
						type = 'crimild::RandomVector3fParticleGenerator',
						attrib = 'velocity',
						minValue = { -2.0, -2.0, -2.0 },
						maxValue = { 2.0, 0.5, 2.0 },
					},
					{
						type = 'crimild::RandomVector3fParticleGenerator',
						attrib = 'acceleration',
						minValue = { 0.0, 0.0, 0.0 },
						maxValue = { 0.0, 0.0, 0.0 },
					},
					{
						type = 'crimild::ColorParticleGenerator',
						minStartColor = { 1.0, 1.0, 1.0, 1.0 },
						maxStartColor = { 1.0, 1.0, 1.0, 1.0 },
						minEndColor = { 0.0, 0.0, 0.0, 1.0 },
						maxEndColor = { 0.0, 0.0, 0.0, 1.0 },
					},
					{
						type = 'crimild::RandomReal32ParticleGenerator',
						attrib = 'uniform_scale_start',
						minValue = 1.0,
						maxValue = 50.0,
					},
					{
						type = 'crimild::RandomReal32ParticleGenerator',
						attrib = 'uniform_scale_end',
						minValue = 100.0,
						maxValue = 300.0,
					},
					{
						type = 'crimild::TimeParticleGenerator',
						minTime = 1.0,
						maxTime = 20.0,
					},
				},
				updaters = {
					{
						type = 'crimild::EulerParticleUpdater',
					},
					{
						type = 'crimild::TimeParticleUpdater',
					},
					{
						type = 'crimild::FloorParticleUpdater',
					},
					{
						type = 'crimild::UniformScaleParticleUpdater',
					},
					{
						type = 'crimild::ColorParticleUpdater',
					},
				},
				renderers = {
					{
						type = 'crimild::PointSpriteParticleRenderer',
						texture = 'assets/textures/smoke.tga',
						blendMode = 'additive',
						cullFaceEnabled = false,
						depthStateEnabled = false,
					},
				},
			},
		},
		transformation = transformation,
	}
end

