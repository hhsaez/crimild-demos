#include <Crimild.hpp>
#include <Crimild_SDL.hpp>
#include <Crimild_Scripting.hpp>
#include <Crimild_Raytracing.hpp>

using namespace crimild;
using namespace crimild::raytracing;
using namespace crimild::sdl;

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
	camera->local().setTranslate( 6.0f, 1.5f, 2.5f );
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
    
    Log::debug( CRIMILD_CURRENT_CLASS_NAME, "Render time: ", c.getDeltaTime(), "s" );

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
	auto settings = crimild::alloc< scripting::LuaSettings >( argc, argv );
	settings->set( "crimild.raytracer.width", 100 );
	settings->set( "crimild.raytracer.height", 100 );
	settings->set( "crimild.raytracer.samples", 1 );
	settings->set( "crimild.raytracer.workers", -1 );
	settings->load( "settings.lua" );
	
	int screenX = settings->get< int >( "crimild.raytracer.width", 200 );
	int screenY = settings->get< int >( "crimild.raytracer.height", 200 );
	int samples = settings->get< int >( "crimild.raytracer.samples", 1 );
	int workers = settings->get< int >( "crimild.raytracer.workers", -1 );

    Log::debug( CRIMILD_CURRENT_CLASS_NAME, "Settings: ", screenX, " ", screenY, " ", samples );

    for ( int i = 0; i < 1; i++ ) {
        crimild::concurrency::JobScheduler jobScheduler;
        jobScheduler.configure( workers );
        jobScheduler.start();

        auto rtResult = rtScene( screenX, screenY, samples );

        jobScheduler.stop();
    }

	return 0;
}

