function handsOnFire( x, y, z )

	local MAX_PARTICLES = 200

	local astroboy = {
		filename = 'assets/models/astroboy.crimild',
		transformation = {
			scale = 70.0,
		},
	}

	local ps = {
		type = 'crimild::Group',
		components = {
			{
				type = 'crimild::ParticleSystemComponent',
				maxParticles = MAX_PARTICLES,
				emitRate = 0.25 * MAX_PARTICLES,
				computeInWorldSpace = true,
				generators = {
					{
						type = 'crimild::NodePositionParticleGenerator',
						node = 'R_middle_01',
						size = { 0.25, 0.25, 0.25 },
					},
					{
						type = 'crimild::RandomVector3fParticleGenerator',
						attrib = 'velocity',
						minValue = { 0.0, 1.0, 0.0 },
						maxValue = { 0.0, 2.5, 0.0 },
					},
					{
						type = 'crimild::DefaultVector3fParticleGenerator',
						attrib = 'acceleration',
						value = { 0.0, 0.0, 0.0 },
					},
					{
						type = 'crimild::RandomReal32ParticleGenerator',
						attrib = 'uniform_scale',
						minValue = 0.25,
						maxValue = 0.75,
					},
					{
						type = 'crimild::TimeParticleGenerator',
						minTime = 0.25,
						maxTime = 0.75,
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
						type = 'crimild::OrientedQuadParticleRenderer',
						texture = 'assets/textures/fire.tga',
						blendMode = 'additive',
						cullFaceEnabled = false,
						depthStateEnabled = false,
					},
				},
			},
		},
	}

	return {
		type = 'crimild::Group',
		nodes = {
			astroboy,
			ps,
		},
		transformation = {
			translate = { x, y, z },
		},
	}
end

