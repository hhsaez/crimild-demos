#include <Crimild.hpp>
#include <Crimild_GLFW.hpp>

namespace crimild {

	class RTMaterial : public NodeComponent {
		CRIMILD_IMPLEMENT_RTTI( crimild::RTMaterial )

	public:
		enum class Type {
			LAMBERTIAN,
			METALLIC,
			DIELECTRIC	
		};
		
	public:
		RTMaterial( void )
		   : _albedo( RGBColorf::ONE ),
			 _type( Type::LAMBERTIAN ),
			 _fuzz( 0.0f ),
			 _refractionIndex( 1.0f )
		{

		}

		virtual ~RTMaterial( void )
		{

		}

		void setType( Type type ) { _type = type; }
		Type getType( void ) const { return _type; }

		const RGBColorf &getAlbedo( void ) const { return _albedo; }
		void setAlbedo( const RGBColorf &albedo ) { _albedo = albedo; }

		float getFuzz( void ) const { return _fuzz; }
		void setFuzz( float value ) { _fuzz = Numericf::min( value, 1.0f ); }

		float getRefractionIndex( void ) const { return _refractionIndex; }
		void setRefractionIndex( float value ) { _refractionIndex = value; }

	private:
		Type _type;
		RGBColorf _albedo;
		float _fuzz;
		float _refractionIndex;
	};

	class RayCaster : public NodeVisitor {
	public:
		struct Result {
			float t;
			Vector3f position;
			Vector3f normal;
			Node *node;
		};

		RayCaster( const Ray3f &ray, float tMin = Numericf::ZERO_TOLERANCE, float tMax = std::numeric_limits< float >::max() )
			: _ray( ray ),
			  _tMin( tMin ),
			  _tMax( tMax )
		{
			
		}

		virtual ~RayCaster( void )
		{
		
		}

		const Ray3f &getRay( void ) const { return _ray; }

		virtual void visitGroup( Group *group )
		{
			if ( true || group->getWorldBound()->testIntersection( getRay() ) ) {
				NodeVisitor::visitGroup( group );
			}
		}

		virtual void visitGeometry( Geometry *geometry )
		{
			Sphere3f s( geometry->getWorldBound()->getCenter(), geometry->getWorldBound()->getRadius() );
			float t = Intersection::find( s, getRay() );
			if ( t > _tMin && t < _tMax ) {
				auto p = getRay().getPointAt( t );
				_candidates.push_back( Result {
					t,
					p,
					( p - s.getCenter() ).getNormalized(),
					geometry
					});
			}
		}

		bool hasMatches( void ) const
		{
			return _candidates.size() > 0;
		}

		const Result &getBestMatch( void )
		{
			_candidates.sort( []( const Result &r1, const Result &r2 ) -> bool {
				return r1.t < r2.t;
			});
			return _candidates.front();
		}

	private:
		Ray3f _ray;
		std::list< Result > _candidates;
		float _tMin;
		float _tMax;
	};

	class RTRenderer : public SharedObject {
	public:
		RTRenderer( int width, int height, int samples )
			: _width( width ),
			  _height( height ),
			  _samples( samples )
		{
			
		}

		virtual ~RTRenderer( void )
		{

		}

		SharedPointer< Image > render( SharedPointer< Node > const &scene, SharedPointer< Camera > camera )
		{
			int bpp = 3;
			std::vector< unsigned char > pixels( _width * _height * bpp );

			for ( int y = _height - 1; y >= 0; y-- ) {
				for ( int x = 0; x < _width; x++ ) {
					RGBColorf c = RGBColorf::ZERO;
					for ( int s = 0; s < _samples; s++ ) {
						float u = ( float ) ( x + Random::generate< float >() ) / ( float ) _width;
						float v = ( float ) ( y + Random::generate< float >() ) / ( float ) _height;

						Ray3f ray;
						camera->getPickRay( u, v, ray );
						c += computeColor( scene, ray );							
					}
					c /= ( float ) _samples;

					// gamma correction
					c = RGBColorf( Numericf::sqrt( c[ 0 ] ), Numericf::sqrt( c[ 1 ] ), Numericf::sqrt( c[ 2 ] ) );
					
					for ( int i = 0; i < bpp; i++ ) {
						pixels[ ( y * _width + x ) * bpp + i ] = ( unsigned char )( 255 * c[ i ] );
					}
				}

#if 1
				Log::Debug << "Rendering... "
						   << ( int )( 100 * ( float )( _height - y ) / ( float ) _height )
						   << Log::End;
#endif
			}

			return crimild::alloc< Image >( _width, _height, bpp, &pixels[ 0 ], Image::PixelFormat::RGB );
		}

	private:
		RGBColorf computeColor( SharedPointer< Node > const &scene, const Ray3f &r, int depth = 0 )
		{
			RayCaster caster( r );
			scene->perform( caster );
			if ( caster.hasMatches() ) {
				auto &hit = caster.getBestMatch();
				auto material = hit.node->getComponent< RTMaterial >();
				Ray3f scattered;
				RGBColorf attenuation = material->getAlbedo();
				RGBColorf color = RGBColorf::ZERO;
				bool visible = true;
				
				switch ( material->getType() ) {
				case RTMaterial::Type::METALLIC: {
					auto reflected = reflect( r.getDirection(), hit.normal );
					reflected += material->getFuzz() * randomInUnitSphere();
					reflected.normalize();
					scattered = Ray3f( hit.position, reflected );
					bool visible = ( scattered.getDirection() * hit.normal > 0 );
					break;
				}
				case RTMaterial::Type::DIELECTRIC: {
					Vector3f outwardNormal;
					Vector3f reflected = reflect( r.getDirection(), hit.normal );
					float refIndex;
					Vector3f refracted;
					float reflectProb;
					float cosine;
					
					if ( r.getDirection() * hit.normal > 0 ) {
						outwardNormal = -hit.normal;
						refIndex = material->getRefractionIndex();
						// why not assume direction is unit-length?
						cosine = ( r.getDirection() * hit.normal ) / ( r.getDirection().getMagnitude() );
						cosine = Numericf::sqrt( 1.0f - refIndex * refIndex * ( 1.0f - cosine * cosine ) );
					}
					else {
						outwardNormal = hit.normal;
						refIndex = 1.0f / material->getRefractionIndex();
						cosine = -( r.getDirection() * hit.normal ) / ( r.getDirection().getMagnitude() );
					}

					if ( refract( r.getDirection(), outwardNormal, refIndex, refracted ) ) {
						reflectProb = schlick( cosine, refIndex );
					}
					else {
						reflectProb = 1.0f;
					}

					if ( Random::generate< float >() < reflectProb ) {
						scattered = Ray3f( hit.position, reflected );
					}
					else {
						scattered = Ray3f( hit.position, refracted );
					}
					break;
				}
				case RTMaterial::Type::LAMBERTIAN: 
				default: {
					Vector3f target = hit.normal + randomInUnitSphere();
					scattered = Ray3f( hit.position, target.getNormalized() );
					break;
				}
				};
		  
				if ( depth < 50 && visible ) {
					auto color = computeColor( scene, scattered, depth + 1 );
					color.times( attenuation );
					return color;
				}
				else {
					return RGBColorf::ZERO;
				}
			}

			Vector3f unitDirection = r.getDirection().getNormalized();
			float t = 0.5f * ( unitDirection.y() + 1.0f );
			RGBColorf output;
			Interpolation::linear( RGBColorf::ONE, RGBColorf( 0.5f, 0.7f, 1.0f ), t, output );
			return output;
		}

		Vector3f randomInUnitSphere( void )
		{
			return Vector3f(
				2.0f * Random::generate< float >() - 1.0f,
				2.0f * Random::generate< float >() - 1.0f,
				2.0f * Random::generate< float >() -1.0f )
			.getNormalized();
		}

		Vector3f reflect( const Vector3f &v, const Vector3f &n )
		{
			return v - 2 * ( v * n ) * n;
		}

		bool refract( const Vector3f &v, const Vector3f &n, float refIndex, Vector3f &refracted )
		{
			Vector3f uv = v.getNormalized();
			Vector3f un = n.getNormalized();
			float dt = uv * un;
			float discriminant = 1.0f - refIndex * refIndex * ( 1 - dt * dt );
			if ( discriminant > 0 ) {
				refracted = refIndex * ( uv - un * dt ) - un * Numericf::sqrt( discriminant );
				return true;
			}

			return false;
		}

		// glass reflectivity
		float schlick( float cosine, float refIndex )
		{
			float r0 = ( 1.0f - refIndex ) / ( 1.0f + refIndex );
			r0 = r0 * r0;
			return r0 + ( 1.0f - r0 ) * Numericf::pow( ( 1.0f - cosine ), 5.0f );
		}

	private:
		int _width;
		int _height;
		int _samples;
	};

}

using namespace crimild;

SharedPointer< Geometry > createSphere( const Vector3f &center, float radius, const RGBColorf &albedo, RTMaterial::Type type, float fuzz = 0.0f, float refIndex = 1.0f )
{
	auto geometry = crimild::alloc< Geometry >();
	geometry->setLocalBound( crimild::alloc< SphereBoundingVolume >() );
	geometry->setWorldBound( crimild::alloc< SphereBoundingVolume >() );
	geometry->attachPrimitive( crimild::alloc< SpherePrimitive >( radius ) );
	geometry->local().setTranslate( center );

	auto material = crimild::alloc< RTMaterial >();
	material->setAlbedo( albedo );
	material->setType( type );
	material->setFuzz( fuzz );
	material->setRefractionIndex( refIndex );
	geometry->attachComponent( material );
	
	return geometry;
}

SharedPointer< Image > rtScene( int nx, int ny, int ns )
{
	auto scene = crimild::alloc< Group >();
	scene->setLocalBound( crimild::alloc< SphereBoundingVolume >() );
	scene->setWorldBound( crimild::alloc< SphereBoundingVolume >() );

	scene->attachNode( createSphere( Vector3f( 0.0f, -1000.0f, 0.0f ), 1000.0f, RGBColorf( 0.5f, 0.5f, 0.5f ), RTMaterial::Type::LAMBERTIAN ) );

	for ( int a = -11; a < 11; a++ ) {
		for ( int b = -11; b < 11; b++ ) {
			float chooseMat = Random::generate< float >();
			Vector3f center( a + 0.9f * Random::generate< float >(), 0.2f, b * 0.9f * Random::generate< float >() );
			if ( ( center - Vector3f( 4.0f, 0.2f, 0.0f ) ).getMagnitude() > 0.9f ) {
				if ( chooseMat < 0.75f ) {
					// diffuse
					scene->attachNode(
						createSphere(
							center,
							0.2f,
							RGBColorf( Random::generate< float >(), Random::generate< float >(), Random::generate< float >() ),
							RTMaterial::Type::LAMBERTIAN
					));
				}
				else if ( chooseMat < 0.9f ) {
					// metallic
					scene->attachNode(
						createSphere(
							center,
							0.2f,
							RGBColorf( Random::generate< float >( 0.5f, 1.0f ), Random::generate< float >( 0.5f, 1.0f ), Random::generate< float >( 0.5f, 1.0f ) ),
							RTMaterial::Type::METALLIC,
							0.5f * Random::generate< float >()
					));
				}
				else {
					// glass
					scene->attachNode(
						createSphere(
							center,
							0.2f,
							RGBColorf( 1.0 ),
							RTMaterial::Type::DIELECTRIC,
							0.0f,
							1.5f
					));
				}
			}
		}
	}
	
	scene->attachNode( createSphere( Vector3f( 0.0f, 1.0f, 0.0f ), 1.0f, RGBColorf( 1.0 ), RTMaterial::Type::DIELECTRIC, 0.0f, 1.5f ) );
	scene->attachNode( createSphere( Vector3f( -4.0f, 1.0f, 0.0f ), 1.0f, RGBColorf( 0.4f, 0.2f, 0.1f ), RTMaterial::Type::LAMBERTIAN ) );
	scene->attachNode( createSphere( Vector3f( 4.0f, 1.0f, 0.0f ), 1.0f, RGBColorf( 0.7f, 0.6f, 0.5f ), RTMaterial::Type::METALLIC, 0.0f ) );

	auto camera = crimild::alloc< Camera >( 60.0f, ( float ) nx / ( float ) ny, 0.1f, 1000.0f );
	camera->local().setTranslate( 2.5f, 1.5f, 6.0f );
	camera->local().lookAt( Vector3f::ZERO );
	camera->setCullingEnabled( false );
	camera->setLocalBound( crimild::alloc< SphereBoundingVolume >() );
	camera->setWorldBound( crimild::alloc< SphereBoundingVolume >() );
	scene->attachNode( camera );

	scene->perform( UpdateWorldState() );
	scene->perform( UpdateRenderState() );

	Clock c;
	c.tick();
	auto renderer = crimild::alloc< RTRenderer >( nx, ny, ns );
	auto result = renderer->render( scene, camera );
	c.tick();

	Log::Debug << "Render time: " << c.getDeltaTime() << "s" << Log::End;

	std::ofstream os( "output.ppm" );
	os << "P3\n"
	   << result->getWidth() << " " << result->getHeight() << "\n"
	   << "255\n";
	
	for ( int j = 0; j < result->getHeight(); j++ ) {
		for ( int i = 0; i < result->getWidth(); i++ ) {
			for ( int k = 0; k < 3; k++ ) {
				os << ( int ) result->getData()[ ( j * result->getWidth() + i ) * 3 + k ] << " ";
			}
			os << "\n";
		}
	}

	os.close();

	return result;
}

int main( int argc, char **argv )
{
	int quality = 1; // med
	if ( argc > 1 ) {
		quality = atoi( argv[ 1 ] );
	}

	int screenX = 160;
	int screenY = 120;
	int samples = 1;

	switch (quality) {
	case 1: // med
		screenX = 320;
		screenY = 240;
		samples = 10;
		break;

	case 2: // high
		screenX = 640;
		screenY = 480;
		samples = 100;
		break;
		
	default:
		break;
	};
		
	auto rtResult = rtScene( screenX, screenY, samples );
	
	auto sim = crimild::alloc< GLSimulation >( "Triangle", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();

	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( crimild::alloc< QuadPrimitive >( screenX, screenY, VertexFormat::VF_P3_UV2 ) );
	auto material = crimild::alloc< Material >();
	material->setColorMap( crimild::alloc< Texture >( rtResult ) );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );
	scene->attachNode( geometry );

	auto camera = crimild::alloc< Camera >();
	camera->local().setTranslate( 0.0f, 0.0f, 1.0f / camera->getFrustum().computeAspect() * screenX );
	scene->attachNode( camera );
		
	sim->setScene( scene );
	return sim->run();


}

