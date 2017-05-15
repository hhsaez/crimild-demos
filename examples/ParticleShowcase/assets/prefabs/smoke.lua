function smoke( x, y, z, computeInWorldSpace )
	
	local MAX_PARTICLES = 200
	
	local ps = {
		type = 'crimild::Group',
		components = {
			{
				type = 'crimild::ParticleSystemComponent',
				maxParticles = MAX_PARTICLES,
				emitRate = 0.25 * MAX_PARTICLES,
				computeInWorldSpace = computeInWorldSpace,
				generators = {
					{
						type = 'crimild::BoxPositionParticleGenerator',
						origin = { 0.0, 0.0, 0.0 },
						size = { 0.05, 0.05, 0.05 },
					},
					{
						type = 'crimild::RandomVector3fParticleGenerator',
						attrib = 'velocity',
						minValue = { 0.0, 1.0, 0.0 },
						maxValue = { 0.0, 2.0, 0.0 },
					},
					{
						type = 'crimild::DefaultVector3fParticleGenerator',
						attrib = 'acceleration',
						value = { 0.0, 0.0, 0.0 },
					},
					{
						type = 'crimild::ColorParticleGenerator',
						minStartColor = { 1.0, 1.0, 1.0, 1.0 },
						maxStartColor = { 1.0, 1.0, 1.0, 1.0 },
						minEndColor = { 1.0, 1.0, 1.0, 0.0 },
						maxEndColor = { 1.0, 1.0, 1.0, 0.0 },
					},
					{
						type = 'crimild::RandomReal32ParticleGenerator',
						attrib = 'uniform_scale',
						minValue = 50.0,
						maxValue = 200.0,
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
						texture = 'assets/textures/smoke.tga',
						blendMode = 'additive',
						cullFaceEnabled = false,
						depthStateEnabled = false,
					},
				},
			},
			{
				type = 'crimild::OrbitComponent',
				x0 = 0.0,
				y0 = 0.0,
				major = 5.0,
				minor = 3.0,
				speed = 2.0,
				gamma = 1.0,
			},
		},
	}

	return {
		type = 'crimild::Group',
		nodes = {
			ps,
		},
		transformation = {
			translate = { x, y, z },
		},
	}
end

