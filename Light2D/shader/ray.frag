#version 460 core

#define RAY_DEPTH 3
#define SAMPLE 16
#define EPSILON 1e-6f
#define TWO_PI 6.28318530718f

in vec2 tex_coords;
out vec4 frag_color;
uniform vec2 viewport_size;

uniform uvec2 noise_size;

uniform int rangle[SAMPLE];
uniform int iteration;
layout (location = 0)uniform sampler2D noise_map;
layout (location = 1)uniform sampler2D frame;

uniform float rotation;

struct scene_sdf
{
	vec2 v2[4];
	float a;
	float luminance;
	float reflectivity;
};

struct ray
{
	vec2 position;
	vec2 direction;
	float coefficient;
	int depth;	
};

uniform scene_sdf static_object[];
uniform scene_sdf dynamic_object[];
// use stack buffer to store rays for iterations and solve ray marching without recursions
// max buffer size equals max reflection/refraction depth
ray ray_buffer[RAY_DEPTH];

struct result
{
	float signed_dist;
	float emissive;
	float reflective;
	float refractive;
	float absorption;
};


float circle_sdf(vec2 p, vec2 c, float r)
{
	return length(p - c) - r;
}

float rectangle_sdf(vec2 p, vec2 c, vec2 hs, float t)
{
	float cos_t = cos(t);
	float sin_t = sin(t);
	mat2 r = 
	{
		{ cos_t, -sin_t },
		{ sin_t, cos_t }
	};
	
	vec2 v = p - c;
	vec2 d = abs(r * v) - hs;
	vec2 a = max(d, 0);

	return min(max(d.x, d.y), 0) + length(a);
}

float cross(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

float segment_sdf(vec2 p, vec2 a, vec2 b, out bool o)
{
	vec2 v = p - a;
	vec2 s = b - a;
	float l = s.x * s.x + s.y * s.y;
	float k = clamp(dot(v, s) / l, 0, 1);
	o = cross(v, s) > 0;
	return length(v - s * k);
}

float triangle_sdf(vec2 p, vec2 v[3])
{
	bool o[3];
	float d = segment_sdf(p, v[0], v[1], o[0]);
	d = min(d, segment_sdf(p, v[1], v[2], o[1]));
	d = min(d, segment_sdf(p, v[2], v[0], o[2]));
	return o[0] && o[1] && o[2] ? -d : d;
}

float regular_polygon_sdf(vec2 p, vec2 c, int e, float r, float t)
{
	vec2 v = p - c;
	float ia = TWO_PI / e;
	// do not use the atan method as it significantly drops fps
	//float k = floor((atan(v.y, v.x) - t) / ia);	
	vec2 a = vec2(cos(t), sin(t));
	float sign_a = cross(v, a);	
	t -= ia;
	vec2 b = vec2(cos(t), sin(t));
	float sign_b = cross(v, b);

	while (sign_b > 0 || sign_a < 0)
	{
		a = b;
		sign_a = sign_b;
		t -= ia;
		b = vec2(cos(t),sin(t));
		sign_b = cross(v, b);
	}

	bool o;
	float d = segment_sdf(v,r * a,r * b, o);
    return o ? -d : d;
}

result union_op(result a, result b)
{
	return a.signed_dist < b.signed_dist ? a : b;
}

result intersect_op(result a, result b)
{
	return a.signed_dist < b.signed_dist ? b : a;
}

result subtract_op(result a, result b)
{
	b.signed_dist = -b.signed_dist;
	return a.signed_dist < b.signed_dist ? b : a;
}

result scene(float x, float y)
{
	vec2 pos = vec2(x, y);
	result a = { circle_sdf(pos, vec2(0.40, 0.7), 0.03), 10, 0f, 0, 0 };
	//result b = {circle_sdf(pos, vec2(1.2, 0.6), 0.05), 0};
	result c = {rectangle_sdf(pos, vec2(0.7, 0.5), vec2(0.18, 0.1), 0),0, 0.2f, 1.5, 4};
	//result c = { boxSDF(pos.x, pos.y, 0.5f, 0.5f, TWO_PI / 16.0f, 0.3f, 0.1f), 1f };
	//result c = { regular_polygon_sdf(pos, vec2(0.9, 0.6), 5, 0.2f, 0.1),0.0f,0.2f, 1.5 };
	//result e = { regular_polygon_sdf(pos, vec2(1.0, 0.4), 3, 0.3f, 0),0.0f,0.2f,1.5 };
	//vec2 v[3] = {vec2(0.85, 0.69), vec2(1.3, 0.4), vec2(0.85, 0.11)};
	//result d = { triangle_sdf(pos, v),0f, 0.2f,1.5 };
	//result d = { segment_sdf(pos, vec2(0.2, 0.2), vec2(0.4, 0.4)) - 0.1,2f };
	return union_op(c,a);//union_op(union_op(a, c), b);//union_op(union_op(a, b), c);
}

vec2 normal(float x, float y)
{
	float dx = (scene(x + EPSILON, y).signed_dist - scene(x - EPSILON, y).signed_dist) / (EPSILON * 2);
	float dy = (scene(x, y + EPSILON).signed_dist - scene(x, y - EPSILON).signed_dist) / (EPSILON * 2);
	vec2 n = normalize(vec2(dx, dy));
	return n;
}

vec2 _reflect(vec2 i, vec2 n)
{
	return i - 2 * dot(i, n) * n;
}

float beerLambert(float a, float d) {
    return exp(-a * d);
}

float march()
{
	float e = 0;		
	int k = 0;

	do
	{
		// pop ray from stack
		ray ra = ray_buffer[k--];

		vec2 o = ra.position;
		float t = 0;
		float s = scene(o.x, o.y).signed_dist > 0 ? 1 : -1;		
		for (int i = 0; i < 64 && t < 2; i++)
		{		
			vec2 p = o + ra.direction * t;

			result r = scene(p.x, p.y);
			if (s * r.signed_dist < EPSILON)
			{
				float att = s < 0 ? beerLambert(r.absorption, t) : 1;
				e += r.emissive * ra.coefficient * att;				
				if (ra.depth > 0 && (r.reflective > 0 || r.refractive > 0))
				{
					vec2 n = s * normal(p.x, p.y);
					float rfl = r.reflective;
					if (r.refractive > 0)
					{
						vec2 rfr = refract(ra.direction, n, s < 0 ? r.refractive : 1/r.refractive);
						if (rfr == 0)
						{
							rfl = 1;
						}
						else
						{
							ray_buffer[++k] = ray(p + rfr * 1e-4, rfr, att * (1-r.reflective), ra.depth - 1);
						}
					}
					if (rfl > 0)
					{
						vec2 rf = reflect(ra.direction, n);
					
						// push reflection ray to stack
						ray_buffer[++k] = ray(p + rf * 1e-4, rf, att * r.reflective, ra.depth - 1);
					}
					
				}
				break;
			}			
			t += s * r.signed_dist;
		}		
	} while (k >= 0);
	return e;
}


float ray_sample(vec2 pos)
{
	float emissive = 0;
//	float noise = texture2D(noise_map, (gl_FragCoord.xy + iteration) / noise_size).x;
//	for (int i = 0; i < SAMPLE; i++)
//	{
//		float angle = TWO_PI * (i * 16 + (iteration + 1) / 2 * (1 - ((iteration + 1) % 2) * 2) + noise) / 256;//SAMPLE;
//		//float angle = (i + noise);
//		//float a =  (i + texture2D(texture1, gl_FragCoord.xy / noise_size).x);
//		//float a =  2*3.1415926 *(i + o*1 + 0*  LFSR_Rand_Gen(pos)) / 64;
//		// push sample ray to stack for ray marching
//		ray_buffer[0] = ray(pos, vec2(cos(angle), sin(angle)), 1, RAY_DEPTH);
//		emissive += march();
//	}

	float noise = texture2D(noise_map, (gl_FragCoord.xy) / noise_size).x;
	for (int i = 0; i < SAMPLE; i++)
	{
		float angle = TWO_PI * (rangle[i] + noise) / 256;//SAMPLE;
		//float angle = (i + noise);
		//float a =  (i + texture2D(texture1, gl_FragCoord.xy / noise_size).x);
		//float a =  2*3.1415926 *(i + o*1 + 0*  LFSR_Rand_Gen(pos)) / 64;
		// push sample ray to stack for ray marching
		ray_buffer[0] = ray(pos, vec2(cos(angle), sin(angle)), 1, RAY_DEPTH);
		emissive += march();
	}
	return emissive / 256;//SAMPLE;
}

void main()
{
	vec2 frag_coord = gl_FragCoord.xy / min(viewport_size.x, viewport_size.y);
	float color = ray_sample(frag_coord);
	//float r = texture2D(texture1, TexCoord).x / 20.f;
	//vec4 c = vec4(normal(frag_coord.x, frag_coord.y) * 0.5 + 0.5,0,1);	
	//frag_color = color.xxxx;
	frag_color = texture2D(frame, tex_coords) + vec4(color.xxx,1);
	//FragColor = texture2D(texture1, gl_FragCoord.xy / noise_size.xy).xxxx;
//	if (trace(gl_FragCoord.xy) - vec2(600,600)) < 50)
//	{
//		FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);	
//	}
//	else
//	{	
//		FragColor = vec4(gl_FragCoord.xy / WindowSize.xy, 1.0f, 1.0f);//vec4(1.0f, 0.5f, 0.2f, 1.0f);	
//	}
}