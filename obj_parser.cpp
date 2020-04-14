/******************************************************************************\
| OpenGL 4 Example Code.                                                       |
| Accompanies written series "Anton's OpenGL 4 Tutorials"                      |
| Email: anton at antongerdelan dot net                                        |
| First version 7 Nov 2013                                                     |
| Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.                |
| See individual libraries' separate legal notices                             |
|******************************************************************************|
| Anton's lazy Wavefront OBJ parser                                            |
| Anton Gerdelan 7 Nov 2013                                                    |
| Notes:                                                                       |
| I ignore MTL files                                                           |
| Mesh MUST be triangulated - quads not accepted                               |
| Mesh MUST contain vertex points, normals, and texture coordinates            |
| Faces MUST come after all other data in the .obj file                        |
\******************************************************************************/
#include "obj_parser.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h" // various extra operations
#include "assimp/scene.h"				// collects data
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mat4 convert_assimp_matrix( aiMatrix4x4 m ) {
	return mat4( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
							 0.0f, m.a4, m.b4, m.c4, m.d4 );
}

/* load a mesh using the assimp library */
bool load_mesh( const char *file_name, GLuint *vao, int *point_count,
								mat4 *bone_offset_mats, int *bone_count ) {
	const aiScene *scene = aiImportFile( file_name, aiProcess_Triangulate );
	if ( !scene ) {
		fprintf( stderr, "ERROR: reading mesh %s\n", file_name );
		return false;
	}
	printf( "  %i animations\n", scene->mNumAnimations );
	printf( "  %i cameras\n", scene->mNumCameras );
	printf( "  %i lights\n", scene->mNumLights );
	printf( "  %i materials\n", scene->mNumMaterials );
	printf( "  %i meshes\n", scene->mNumMeshes );
	printf( "  %i textures\n", scene->mNumTextures );

	/* get first mesh in file only */
	const aiMesh *mesh = scene->mMeshes[0];
	printf( "    %i vertices in mesh[0]\n", mesh->mNumVertices );

	/* pass back number of vertex points in mesh */
	*point_count = mesh->mNumVertices;

	/* generate a VAO, using the pass-by-reference parameter that we give to the
	function */
	glGenVertexArrays( 1, vao );
	glBindVertexArray( *vao );

	/* we really need to copy out all the data from AssImp's funny little data
	structures into pure contiguous arrays before we copy it into data buffers
	because assimp's texture coordinates are not really contiguous in memory.
	i allocate some dynamic memory to do this. */
	GLfloat *points = NULL;		 // array of vertex points
	GLfloat *normals = NULL;	 // array of vertex normals
	GLfloat *texcoords = NULL; // array of texture coordinates
	GLint *bone_ids = NULL;		 // array of bone IDs
	if ( mesh->HasPositions() ) {
		points = (GLfloat *)malloc( *point_count * 3 * sizeof( GLfloat ) );
		for ( int i = 0; i < *point_count; i++ ) {
			const aiVector3D *vp = &( mesh->mVertices[i] );
			points[i * 3] = (GLfloat)vp->x;
			points[i * 3 + 1] = (GLfloat)vp->y;
			points[i * 3 + 2] = (GLfloat)vp->z;
		}
	}
	if ( mesh->HasNormals() ) {
		normals = (GLfloat *)malloc( *point_count * 3 * sizeof( GLfloat ) );
		for ( int i = 0; i < *point_count; i++ ) {
			const aiVector3D *vn = &( mesh->mNormals[i] );
			normals[i * 3] = (GLfloat)vn->x;
			normals[i * 3 + 1] = (GLfloat)vn->y;
			normals[i * 3 + 2] = (GLfloat)vn->z;
		}
	}
	if ( mesh->HasTextureCoords( 0 ) ) {
		texcoords = (GLfloat *)malloc( *point_count * 2 * sizeof( GLfloat ) );
		for ( int i = 0; i < *point_count; i++ ) {
			const aiVector3D *vt = &( mesh->mTextureCoords[0][i] );
			texcoords[i * 2] = (GLfloat)vt->x;
			texcoords[i * 2 + 1] = (GLfloat)vt->y;
		}
	}


	/* extract bone weights */
	if ( mesh->HasBones() ) {
		*bone_count = (int)mesh->mNumBones;
		/* an array of bones names. max 256 bones, max name length 64 */
		char bone_names[256][64];

		/* here I allocate an array of per-vertex bone IDs.
		each vertex must know which bone(s) affect it
		here I simplify, and assume that only one bone can affect each vertex,
		so my array is only one-dimensional
		*/
		bone_ids = (int *)malloc( *point_count * sizeof( int ) );


		for ( int b_i = 0; b_i < *bone_count; b_i++ ) {
			const aiBone *bone = mesh->mBones[b_i];

			/* get bone names */
			strcpy( bone_names[b_i], bone->mName.data );
			printf( "bone_names[%i]=%s\n", b_i, bone_names[b_i] );

			/* get [inverse] offset matrix for each bone */
			bone_offset_mats[b_i] = convert_assimp_matrix( bone->mOffsetMatrix );

			/* get bone weights
			we can just assume weight is always 1.0, because we are just using 1 bone
			per vertex. but any bone that affects a vertex will be assigned as the
			vertex' bone_id */
			int num_weights = (int)bone->mNumWeights;
			// none！！

		} // endfor
	}		// endif


	/* copy mesh data into VBOs */
	if ( mesh->HasPositions() ) {
		GLuint vbo;
		glGenBuffers( 1, &vbo );
		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, 3 * *point_count * sizeof( GLfloat ), points,
									GL_STATIC_DRAW );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
		glEnableVertexAttribArray( 0 );
		free( points );
	}
	if ( mesh->HasNormals() ) {
		GLuint vbo;
		glGenBuffers( 1, &vbo );
		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, 3 * *point_count * sizeof( GLfloat ), normals,
									GL_STATIC_DRAW );
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, NULL );
		glEnableVertexAttribArray( 1 );
		free( normals );
	}
	if ( mesh->HasTextureCoords( 0 ) ) {
		GLuint vbo;
		glGenBuffers( 1, &vbo );
		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, 2 * *point_count * sizeof( GLfloat ), texcoords,
									GL_STATIC_DRAW );
		glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 0, NULL );
		glEnableVertexAttribArray( 2 );
		free( texcoords );
	}
	if ( mesh->HasTangentsAndBitangents() ) {
		// NB: could store/print tangents here
	}
	if ( mesh->HasBones() ) {
		GLuint vbo;
		glGenBuffers( 1, &vbo );
		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, *point_count * sizeof( GLint ), bone_ids,
									GL_STATIC_DRAW );
		glVertexAttribIPointer( 3, 1, GL_INT, 0, NULL );
		glEnableVertexAttribArray( 3 );
		free( bone_ids );
	}

	aiReleaseImport( scene );
	printf( "mesh loaded\n" );

	return true;
}

bool load_obj_file( const char *file_name, float *&points, float *&tex_coords,
										float *&normals, int &point_count ) {

	float *unsorted_vp_array = NULL;
	float *unsorted_vt_array = NULL;
	float *unsorted_vn_array = NULL;
	int current_unsorted_vp = 0;
	int current_unsorted_vt = 0;
	int current_unsorted_vn = 0;

	FILE *fp = fopen( file_name, "r" );
	if ( !fp ) {
		fprintf( stderr, "ERROR: could not find file %s\n", file_name );
		return false;
	}

	// first count points in file so we know how much mem to allocate
	point_count = 0;
	int unsorted_vp_count = 0;
	int unsorted_vt_count = 0;
	int unsorted_vn_count = 0;
	int face_count = 0;
	char line[1024];
	while ( fgets( line, 1024, fp ) ) {
		if ( line[0] == 'v' ) {
			if ( line[1] == ' ' ) {
				unsorted_vp_count++;
			} else if ( line[1] == 't' ) {
				unsorted_vt_count++;
			} else if ( line[1] == 'n' ) {
				unsorted_vn_count++;
			}
		} else if ( line[0] == 'f' ) {
			face_count++;
		}
	}
	printf( "found %i vp %i vt %i vn unique in obj. allocating memory...\n",
					unsorted_vp_count, unsorted_vt_count, unsorted_vn_count );
	unsorted_vp_array = (float *)malloc( unsorted_vp_count * 3 * sizeof( float ) );
	unsorted_vt_array = (float *)malloc( unsorted_vt_count * 2 * sizeof( float ) );
	unsorted_vn_array = (float *)malloc( unsorted_vn_count * 3 * sizeof( float ) );
	points = (float *)malloc( 3 * face_count * 3 * sizeof( float ) );
	tex_coords = (float *)malloc( 3 * face_count * 2 * sizeof( float ) );
	normals = (float *)malloc( 3 * face_count * 3 * sizeof( float ) );
	printf( "allocated %i bytes for mesh\n",
					(int)( 3 * face_count * 8 * sizeof( float ) ) );

	rewind( fp );
	while ( fgets( line, 1024, fp ) ) {
		// vertex
		if ( line[0] == 'v' ) {

			// vertex point
			if ( line[1] == ' ' ) {
				float x, y, z;
				x = y = z = 0.0f;
				sscanf( line, "v %f %f %f", &x, &y, &z );
				unsorted_vp_array[current_unsorted_vp * 3] = x;
				unsorted_vp_array[current_unsorted_vp * 3 + 1] = y;
				unsorted_vp_array[current_unsorted_vp * 3 + 2] = z;
				current_unsorted_vp++;

				// vertex texture coordinate
			} else if ( line[1] == 't' ) {
				float s, t;
				s = t = 0.0f;
				sscanf( line, "vt %f %f", &s, &t );
				unsorted_vt_array[current_unsorted_vt * 2] = s;
				unsorted_vt_array[current_unsorted_vt * 2 + 1] = t;
				current_unsorted_vt++;

				// vertex normal
			} else if ( line[1] == 'n' ) {
				float x, y, z;
				x = y = z = 0.0f;
				sscanf( line, "vn %f %f %f", &x, &y, &z );
				unsorted_vn_array[current_unsorted_vn * 3] = x;
				unsorted_vn_array[current_unsorted_vn * 3 + 1] = y;
				unsorted_vn_array[current_unsorted_vn * 3 + 2] = z;
				current_unsorted_vn++;
			}

			// faces
		} else if ( line[0] == 'f' ) {
			// work out if using quads instead of triangles and print a warning
			int slashCount = 0;
			int len = strlen( line );
			for ( int i = 0; i < len; i++ ) {
				if ( line[i] == '/' ) {
					slashCount++;
				}
			}
			if ( slashCount != 6 ) {
				fprintf( stderr,
								 "ERROR: file contains quads or does not match v vp/vt/vn layout - \
					make sure exported mesh is triangulated and contains vertex points, \
					texture coordinates, and normals\n" );
				return false;
			}

			int vp[3], vt[3], vn[3];
			sscanf( line, "f %i/%i/%i %i/%i/%i %i/%i/%i", &vp[0], &vt[0], &vn[0], &vp[1],
							&vt[1], &vn[1], &vp[2], &vt[2], &vn[2] );

			/* start reading points into a buffer. order is -1 because obj starts from
				 1, not 0 */
			// NB: assuming all indices are valid
			for ( int i = 0; i < 3; i++ ) {
				if ( ( vp[i] - 1 < 0 ) || ( vp[i] - 1 >= unsorted_vp_count ) ) {
					fprintf( stderr, "ERROR: invalid vertex position index in face\n" );
					return false;
				}
				if ( ( vt[i] - 1 < 0 ) || ( vt[i] - 1 >= unsorted_vt_count ) ) {
					fprintf( stderr, "ERROR: invalid texture coord index %i in face.\n",
									 vt[i] );
					return false;
				}
				if ( ( vn[i] - 1 < 0 ) || ( vn[i] - 1 >= unsorted_vn_count ) ) {
					printf( "ERROR: invalid vertex normal index in face\n" );
					return false;
				}
				points[point_count * 3] = unsorted_vp_array[( vp[i] - 1 ) * 3];
				points[point_count * 3 + 1] = unsorted_vp_array[( vp[i] - 1 ) * 3 + 1];
				points[point_count * 3 + 2] = unsorted_vp_array[( vp[i] - 1 ) * 3 + 2];
				tex_coords[point_count * 2] = unsorted_vt_array[( vt[i] - 1 ) * 2];
				tex_coords[point_count * 2 + 1] = unsorted_vt_array[( vt[i] - 1 ) * 2 + 1];
				normals[point_count * 3] = unsorted_vn_array[( vn[i] - 1 ) * 3];
				normals[point_count * 3 + 1] = unsorted_vn_array[( vn[i] - 1 ) * 3 + 1];
				normals[point_count * 3 + 2] = unsorted_vn_array[( vn[i] - 1 ) * 3 + 2];
				point_count++;
			}
		}
	}
	fclose( fp );
	free( unsorted_vp_array );
	free( unsorted_vn_array );
	free( unsorted_vt_array );
	printf( "allocated %i points\n", point_count );
	return true;
}
