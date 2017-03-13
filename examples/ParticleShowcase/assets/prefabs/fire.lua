function fire( x, y, z )
	local MAX_PARTICLES = 300
	
	return {
		type = 'crimild::Group',
		components = {
			{
				type = 'crimild::ParticleSystemComponent',
				maxParticles = MAX_PARTICLES,
				emitRate = 0.75 * MAX_PARTICLES,
				preWarmTime = 1.0,
				generators = {
					{
						type = 'crimild::BoxPositionParticleGenerator',
						origin = { 0.0, 0.0, 0.0 },
						size = { 2.0, 0.25, 2.0 },
					},
					{
						type = 'crimild::RandomVector3fParticleGenerator',
						attrib = 'velocity',
						min = { 0.0, 1.0, 0.0 },
						max = { 0.0, 5.0, 0.0 },
					},
					{
						type = 'crimild::DefaultVector3fParticleGenerator',
						attrib = 'acceleration',
						value = { 0.0, 0.0, 0.0 },
					},
					{
						type = 'crimild::ColorParticleGenerator',
						minStartColor = { 1.0, 0.0, 0.0, 1.0 },
						maxStartColor = { 1.0, 1.0, 0.0, 1.0 },
						minEndColor = { 1.0, 1.0, 1.0, 0.0 },
						maxEndColor = { 1.0, 1.0, 1.0, 0.0 },
					},
					{
						type = 'crimild::RandomReal32ParticleGenerator',
						attrib = 'uniform_scale',
						min = 50.0,
						max = 200.0,
					},
					{
						type = 'crimild::TimeParticleGenerator',
						minTime = 1.0,
						maxTime = 2.0,
					},
				},
				updaters = {
					{
						type = 'crimild::EulerParticleUpdater',
					},
					{
						type = 'crimild::TimeParticleUpdater',
					},
				},
				renderers = {
					{
						type = 'crimild::PointSpriteParticleRenderer',
						texture = 'assets/textures/fire.tga',
						blendMode = 'additive',
						cullFaceEnabled = false,
						depthStateEnabled = false,
					},
				},
			},
		},
		transformation = {
			translate = { x, y, z },
		},
	}
end


