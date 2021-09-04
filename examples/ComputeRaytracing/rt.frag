const auto FRAG_SRC = R"(

    layout( local_size_x = 32, local_size_y = 32 ) in;
    layout( set = 0, binding = 0, rgba8 ) uniform image2D resultImage;

    layout ( set = 0, binding = 1 ) uniform RenderSettings {
        uint sampleCount;
        uint maxSamples;
        uint maxBounces;
        float aperture;
        float focusDist;
    } uRenderSettings;

    layout ( set = 0, binding = 2 ) uniform Uniforms {
        uint sampleCount;
        uint maxSamples;
        uint seedStart;
        float tanHalfFOV;
        float aspectRatio;
        float aperture;
        float focusDist;
        mat4 proj;
        mat4 view;
    };

    uint seed = 0;
    int flat_idx = 0;


    struct Sphere {
        mat4 invWorld;
        uint materialID;
    };

    struct Material {
        vec3 albedo;
        float metallic;
        float roughness;
        float ambientOcclusion;
        float transmission;
        float indexOfRefraction;
        vec3 emissive;
    };

#define MAX_SPHERE_COUNT 500
#define MAX_MATERIAL_COUNT 500

    layout ( set = 0, binding = 3 ) uniform SceneUniforms {
        Sphere spheres[ MAX_SPHERE_COUNT ];
        int sphereCount;
        Material materials[ MAX_MATERIAL_COUNT ];
        int materialCount;
    } uScene;

    struct HitRecord {
        bool hasResult;
        float t;
        uint materialID;
        vec3 point;
        vec3 normal;
        bool frontFace;
    };

    struct Ray {
        vec3 origin;
        vec3 direction;
    };

    struct Camera {
        vec3 origin;
        vec3 lowerLeftCorner;
        vec3 horizontal;
        vec3 vertical;
        vec3 u;
        vec3 v;
        vec3 w;
        float lensRadius;
    };

    void encrypt_tea(inout uvec2 arg)
    {
        uvec4 key = uvec4(0xa341316c, 0xc8013ea4, 0xad90777d, 0x7e95761e);
        uint v0 = arg[0], v1 = arg[1];
        uint sum = 0u;
        uint delta = 0x9e3779b9u;

        for(int i = 0; i < 32; i++) {
            sum += delta;
            v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
            v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
        }
        arg[0] = v0;
        arg[1] = v1;
    }

    float getRandom()
    {
        uvec2 arg = uvec2( flat_idx, seed++);
        encrypt_tea(arg);
        vec2 r = fract(vec2(arg) / vec2(0xffffffffu));
        return r.x;
    }

    float getRandomRange( float min, float max )
    {
        return min + getRandom() * ( max - min );
    }

    vec3 getRandomVec3()
    {
        return vec3(
            getRandom(),
            getRandom(),
            getRandom()
        );
    }

    vec3 getRandomVec3Range( float min, float max )
    {
        return vec3(
            getRandomRange( min, max ),
            getRandomRange( min, max ),
            getRandomRange( min, max )
        );
    }

    vec3 getRandomInUnitSphere()
    {
        while ( true ) {
            vec3 p = getRandomVec3Range( -1.0, 1.0 );
            if ( dot( p, p ) < 1.0 ) {
                return p;
            }
        }
        return vec3( 0 );
    }

    vec3 getRandomInUnitDisc()
    {
        while ( true ) {
            vec3 p = vec3(
                getRandomRange( -1.0, 1.0 ),
                getRandomRange( -1.0, 1.0 ),
                0.0
            );
            if ( dot( p, p ) >= 1.0 ) {
                break;
            }
            return p;
        }
    }

    vec3 getRandomUnitVector()
    {
        return normalize( getRandomInUnitSphere() );
    }

    vec3 getRandomInHemisphere( vec3 N )
    {
        vec3 inUnitSphere = getRandomInUnitSphere();
        if ( dot( inUnitSphere, N ) > 0.0 ) {
            return inUnitSphere;
        } else {
            return -inUnitSphere;
        }
    }

    HitRecord setFaceNormal( Ray ray, vec3 N, HitRecord rec )
    {
        rec.frontFace = dot( ray.direction, N ) < 0;
        rec.normal = rec.frontFace ? N : -N;
        return rec;
    }

    vec3 rayAt( Ray ray, float t ) {
        return ray.origin + t * ray.direction;
    }

    struct Scattered {
        bool hasResult;
        bool isEmissive;
        Ray ray;
        vec3 attenuation;
    };

    bool isZero( vec3 v )
    {
        float s = 0.00001;
        return abs( v.x ) < s && abs( v.y ) < s && abs( v.z ) < s;
    }

    float reflectance( float cosine, float refIdx )
    {
        float r0 = ( 1.0 - refIdx ) / ( 1.0 + refIdx );
        r0 = r0 * r0;
        return r0 + ( 1.0 - r0 ) * pow( ( 1.0 - cosine ), 5.0 );
    }

    Scattered scatter( Material material, Ray ray, HitRecord rec )
    {
        Scattered scattered;
        scattered.hasResult = false;
        scattered.isEmissive = false;

        if ( material.transmission > 0 ) {
            float ratio = rec.frontFace ? ( 1.0 / material.indexOfRefraction ) : material.indexOfRefraction;
            vec3 D = normalize( ray.direction );
            vec3 N = rec.normal;
            float cosTheta = min( dot( -D, N ), 1.0 );
            float sinTheta = sqrt( 1.0 - cosTheta * cosTheta );
            bool cannotRefract = ratio * sinTheta > 1.0;
            if ( cannotRefract || reflectance( cosTheta, ratio ) > getRandom() ) {
                D = reflect( D, N );
            } else {
                D = refract( D, N, ratio );
            }
            scattered.ray.origin = rec.point;
            scattered.ray.direction = D;
            scattered.attenuation = vec3( 1.0 );
            scattered.hasResult = true;
        } else if ( material.metallic > 0 ) {
            vec3 reflected = reflect( normalize( ray.direction ), rec.normal );
            scattered.ray.origin = rec.point;
            scattered.ray.direction = reflected + material.roughness * getRandomInUnitSphere();
            scattered.attenuation = material.albedo;
            scattered.hasResult = dot( scattered.ray.direction, rec.normal ) > 0.0;
        } else if ( !isZero( material.emissive ) ) {
            scattered.isEmissive = true;
            scattered.hasResult = true;
            scattered.attenuation = material.emissive;
        } else {
            vec3 scatterDirection = rec.normal + getRandomUnitVector();
            if ( isZero( scatterDirection ) ) {
                scatterDirection = rec.normal;
            }
            scattered.ray.origin = rec.point;
            scattered.ray.direction = scatterDirection;
            scattered.attenuation = material.albedo;
            scattered.hasResult = true;
        }

        return scattered;
    }

    HitRecord hitSphere( Sphere sphere, Ray worldRay, float tMin, float tMax ) {
        HitRecord rec;
        Ray ray;
        ray.origin = ( sphere.invWorld * vec4( worldRay.origin, 1.0 ) ).xyz;
        ray.direction = ( sphere.invWorld * vec4( worldRay.direction, 0.0 ) ).xyz;

        vec3 CO = ray.origin;
        float a = dot( ray.direction, ray.direction );
        float b = 2.0 * dot( ray.direction, CO );
        float c = dot( CO, CO ) - 1.0;

        float d = b * b - 4.0 * a * c;
        if ( d < 0 ) {
            rec.hasResult = false;
            return rec;
        }

        float root = sqrt( d );
        float t = ( -b - root ) / ( 2.0 * a );
        if ( t < tMin || t > tMax ) {
            t = ( -b + root ) / ( 2.0 * a );
            if ( t < tMin || t > tMax ) {
                rec.hasResult = false;
                return rec;
            }
        }

        rec.hasResult = true;
        rec.t = t;
        rec.materialID = sphere.materialID;
        vec3 P = rayAt( ray, t );
        mat4 world = inverse( sphere.invWorld );
        rec.point = ( world * vec4( P, 1.0 ) ).xyz;
        vec3 normal = normalize( transpose( mat3( sphere.invWorld ) ) * P );
        return setFaceNormal( ray, normal, rec );
    }

    HitRecord hitScene( Ray ray, float tMin, float tMax )
    {
        HitRecord hit;
        hit.t = tMax;
        hit.hasResult = false;
        for ( int i = 0; i < uScene.sphereCount; i++ ) {
            HitRecord candidate = hitSphere( uScene.spheres[ i ], ray, tMin, hit.t );
            if ( candidate.hasResult ) {
                hit = candidate;
            }
        }
        return hit;
    }

    Camera createCamera( float aspectRatio )
    {
        float h = tanHalfFOV;
        vec2 viewport = vec2( 2.0 * aspectRatio * h, 2.0 * h );

        vec3 lookFrom = vec3( 13, 2, 3 );
        vec3 lookAt = vec3( 0, 0, 0 );
        vec3 vUp = vec3( 0, 1, 0 );

        Camera camera;
        camera.w = -normalize( lookFrom - lookAt );
        camera.u = normalize( cross( vUp, camera.w ) );
        camera.v = cross( camera.w, camera.u );
        // camera.u = ( view * vec4( 1, 0, 0, 0 ) ).xyz;
        // camera.v = ( view * vec4( 0, 1, 0, 0 ) ).xyz;
        // camera.w = ( view * vec4( 0, 0, -1, 0 ) ).xyz;

        // camera.origin = ( view * vec4( 0, 0, 0, 1 ) ).xyz;
        camera.origin = lookFrom;
        camera.horizontal = focusDist * viewport.x * camera.u;
        camera.vertical = focusDist * viewport.y * camera.v;
        camera.lowerLeftCorner = camera.origin - camera.horizontal / 2.0 - camera.vertical / 2.0 + focusDist * camera.w;
        camera.lensRadius = aperture / 2.0;
        return camera;
    }

    Ray getCameraRay( Camera camera, float u, float v )
    {
        /*
          vec3 rd = camera.lensRadius * getRandomInUnitDisc();
          vec3 offset = camera.u * rd.x + camera.v * rd.y;

          Ray ray;
          ray.origin = camera.origin + offset;
          ray.direction = camera.lowerLeftCorner + u * camera.horizontal + v * camera.vertical - camera.origin - offset;
          return ray;
        */

        float x = 2.0 * u - 1.0;
        float y = 2.0 * v - 1.0;
        vec4 rayClip = vec4( x, y, -1, 1 );
        vec4 rayEye = inverse( proj ) * rayClip;

        Ray ray;
        ray.origin = ( inverse( view ) * vec4( 0, 0, 0, 1 ) ).xyz;//camera.origin;
        ray.direction = ( inverse( mat3( view ) ) * rayEye.xyz ).xyz;
        return ray;
    }

    vec3 rayColor( Ray ray ) {
        uint maxDepth = uRenderSettings.maxBounces;

        float multiplier = 1.0;
        float tMin = 0.001;
        float tMax = 9999.9;
        vec3 attenuation = vec3( 1.0 );

        vec3 color = vec3( 1.0 );

        int depth = 0;
        while ( true ) {
            if ( depth >= maxDepth ) {
                return vec3( 0 );
            }

            HitRecord hit = hitScene( ray, tMin, tMax );
            if ( !hit.hasResult ) {
                // no hit. use background color
                vec3 D = normalize( ray.direction );
                float t = 0.5 * ( D.y + 1.0 );
                vec3 backgroundColor = ( 1.0 - t ) * vec3( 1.0, 1.0, 1.0 ) + t * vec3( 0.5, 0.7, 1.0 );
                color *= backgroundColor;
                return color;
            } else {
                Scattered scattered = scatter( uScene.materials[ hit.materialID ], ray, hit );
                if ( scattered.hasResult ) {
                    if ( scattered.isEmissive ) {
                        return scattered.attenuation;
                    }
                    color *= scattered.attenuation;
                    ray = scattered.ray;
                    ++depth;
                } else {
                    return vec3( 0 );
                }
            }
        }

        // never happens
        return vec3( 0 );
    }

    vec3 gammaCorrection( vec3 color, int samplesPerPixel )
    {
        float scale = 1.0 / float( samplesPerPixel );
        return sqrt( scale * color );
    }

    void main() {
        seed = seedStart;

        if ( sampleCount >= uRenderSettings.maxSamples ) {
            return;
        }

        flat_idx = int(dot(gl_GlobalInvocationID.xy, vec2(1, 4096)));

        vec2 size = imageSize( resultImage );
        float aspectRatio = size.x / size.y;

        if ( gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y ) {
            return;
        }

        Camera camera = createCamera( aspectRatio );

        vec2 uv = gl_GlobalInvocationID.xy;
        uv += vec2( getRandom(), getRandom() );
        uv /= ( size.xy - vec2( 1 ) );
        uv.y = 1.0 - uv.y;
        Ray ray = getCameraRay( camera, uv.x, uv.y );
        vec3 color = rayColor( ray );

        vec3 destinationColor = imageLoad( resultImage, ivec2( gl_GlobalInvocationID.xy ) ).rgb;
        color = ( destinationColor * float( sampleCount - 1 ) + color ) / float( sampleCount );

        if ( sampleCount == 0 ) {
            color = vec3( 0 );
        }

        imageStore( resultImage, ivec2( gl_GlobalInvocationID.xy ), vec4( color, 1.0 ) );
    }

)";
