function animated_fire( x, y, z )
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
						minValue = { 0.0, 1.0, 0.0 },
						maxValue = { 0.0, 5.0, 0.0 },
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
						minValue = 1.0,
						maxValue = 2.0,
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
					{
						type = 'crimild::CameraSortParticleUpdater',
					},
				},
				renderers = {
					{
						type = 'crimild::AnimatedSpriteParticleRenderer',
						texture = 'assets/textures/flames.tga',
						spriteSheetSize = { 4.0, 4.0 },
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


