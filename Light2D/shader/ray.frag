#version 460 core

#define RAY_DEPTH 5
#define SAMPLE 16
#define ITERATION 32
#define EPSILON 1e-6f
#define RFR_OFFSET 1e-4
#define RFL_OFFSET 1e-5
#define TWO_PI 6.28318530718f

in vec2 tex_coords;
layout (location = 0) out vec4 frag_color;
uniform vec2 viewport_size;

struct light_source
{
	vec2 position;
	float radius;
	vec3 luminance;
};

uniform light_source light1;

uniform uvec2 noise_size;
uniform int iteration_count;

uniform int rangle[SAMPLE];
layout (location = 0)uniform sampler2D noise_map;
layout (location = 1)uniform sampler2D frame_canvas;

struct ray
{
	vec2 position;
	vec2 direction;
	vec3 coefficient;
	int depth;	
};

// use stack buffer to store rays for iterations and solve ray marching without recursions
// max buffer size equals max reflection/refraction depth
// with RGB colored ray, need to plus 2 to buffer size to accommodate refrected rays
ray ray_buffer[RAY_DEPTH + 2];

struct result
{
	float signed_dist;
	vec3 emissive;
	vec3 reflective; // r0
	vec3 refractive;
	vec3 absorption;
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

// o = winding order
float segment_sdf(vec2 p, vec2 a, vec2 b, in out float o)
{
	vec2 v = p - a;
	vec2 s = b - a;
	float l = s.x * s.x + s.y * s.y;
	float k = clamp(dot(v, s) / l, 0, 1);
	o = min(o, sign(cross(v, s)));
	return length(v - s * k);
}

float triangle_sdf(vec2 p, vec2 v[3])
{
	float o = 1;
	float d = segment_sdf(p, v[0], v[1], o);
	d = min(d, segment_sdf(p, v[1], v[2], o));
	d = min(d, segment_sdf(p, v[2], v[0], o));
	return o * -d;
}

float regular_triangle_sdf(vec2 p, vec2 c, float r, float t)
{
	float ia = TWO_PI / 3;
	vec2[3] e =
	{
		vec2(cos(t), sin(t)),
		vec2(cos(t - ia), sin(t - ia)),
		vec2(cos(t + ia), sin(t + ia))
	};
	return triangle_sdf((p - c) / r, e) * r;
}

float regular_pentagon_sdf(vec2 p, vec2 c, float r, float t)
{
	vec2 v = (p - c) / r;
	float ia = TWO_PI / 5;
	float ia2 = ia * 2;

	vec2[5] e =
	{
		vec2(cos(t), sin(t)),
		vec2(cos(t - ia), sin(t - ia)),
		vec2(cos(t - ia2), sin(t - ia2)),
		vec2(cos(t + ia2), sin(t + ia2)),
		vec2(cos(t + ia), sin(t + ia))
	};

	float o = 1;
	float d = segment_sdf(v, e[0], e[1], o);
	d = min(d, segment_sdf(v, e[1], e[2], o));
	d = min(d, segment_sdf(v, e[2], e[3], o));
	d = min(d, segment_sdf(v, e[3], e[4], o));
	d = min(d, segment_sdf(v, e[4], e[0], o));
	return o * -d * r;
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
	result light = 
	{
		circle_sdf(pos,light1.position/min(viewport_size.x,viewport_size.y),light1.radius),
		light1.luminance,
		vec3(0),
		vec3(0),
		vec3(0)
	};

	result square = {
		rectangle_sdf(pos,vec2(1.0,0.24), vec2(0.1,0.1),0.12),
		vec3(0),
		vec3(0.01,0.03,0.06),
		vec3(1.3,1.31,1.33),
		vec3(5,1,1)
	};

	result pentangon = {
		regular_pentagon_sdf(pos,vec2(1.2,0.76),0.12f,0.7), // distance
		vec3(0), // emissive
		vec3(0.11,0.07,0.01), // reflective
		vec3(1.5,1.52,1.55), // refractive
		vec3(1,2,6) // absorption
	};

	result circle1 = {
		circle_sdf(pos,vec2(0.41,0.69),0.12),
		vec3(0),
		vec3(0.08,0.22,0.07),
		vec3(1.5,1.52,1.55),
		vec3(1,7,3)
	};

	result circle2 = {
		circle_sdf(pos,vec2(0.6,0.59),0.05),
		vec3(0),
		vec3(0.28,0.25,0.05),
		vec3(1.49,1.50,1.56),
		vec3(10,10,1)
	};

	//return union_op(union_op(light, circle1), circle2);
	return union_op(union_op(union_op(union_op(light, pentangon), square), circle1), circle2);
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

vec3 beerLambert(vec3 a, float d) {
    return vec3(exp(-a.x * d), exp(-a.y * d), exp(-a.z * d));
}

float fresnelSchlick(float r0, float cos_i) {
    float a = 1.0f - cos_i;
    float aa = a * a;
    return r0 + (1.0f - r0) * aa * aa * a;
}

vec3 march()
{
	vec3 e = vec3(0);
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
				if (s < 0)
				{
					ra.coefficient *=  beerLambert(r.absorption, t);
				}				
				e += r.emissive * ra.coefficient;				
				if (ra.depth > 0)
				{
					vec2 n = s * normal(p.x, p.y);
					vec3 eta = s < 0 ? r.refractive : 1 / r.refractive;
					float cos_i = -dot(ra.direction, n);

					if (ra.coefficient[0] > 0 && r.refractive[0] > 0)
					{
						vec2 rf = refract(ra.direction, n, eta[0]);
						if (rf == 0)
						{
							r.reflective[0] = 1; // total internal reflection
						}
						else
						{
							r.reflective[0] = fresnelSchlick(r.reflective[0], eta[0] < 1 ? cos_i : -dot(rf, n));
							vec3 c = vec3(1 - r.reflective[0], 0, 0);							
							ray_buffer[++k] = ray(p + rf * RFR_OFFSET, rf, ra.coefficient * c, ra.depth - 1);
						} 
					}

					if (ra.coefficient[1] > 0 && r.refractive[1] > 0)
					{
						vec2 rf = refract(ra.direction, n, eta[1]);
						if (rf == 0)
						{
							r.reflective[1] = 1; // total internal reflection
						}
						else
						{
							r.reflective[1] = fresnelSchlick(r.reflective[1], eta[1] < 1 ? cos_i : -dot(rf, n));
							vec3 c = vec3(0, 1 - r.reflective[1], 0);							
							ray_buffer[++k] = ray(p + rf * RFR_OFFSET, rf, ra.coefficient * c, ra.depth - 1);
						} 
					}

					if (ra.coefficient[2] > 0 && r.refractive[2] > 0)
					{
						vec2 rf = refract(ra.direction, n, eta[2]);
						if (rf == 0)
						{
							r.reflective[2] = 1; // total internal reflection
						}
						else
						{
							r.reflective[2] = fresnelSchlick(r.reflective[2], eta[2] < 1 ? cos_i : -dot(rf, n));
							vec3 c = vec3(0, 0, 1 - r.reflective[2]);							
							ray_buffer[++k] = ray(p + rf * RFR_OFFSET, rf, ra.coefficient * c, ra.depth - 1);
						} 
					}

					if (length(r.reflective) > 0)
					{
						vec2 rf = reflect(ra.direction, n);
					
						// push reflection ray to stack
						ray_buffer[++k] = ray(p + rf * RFL_OFFSET, rf, ra.coefficient * r.reflective, ra.depth - 1);
					}					
				}
				break;
			}			
			t += s * r.signed_dist;
		}		
	} while (k >= 0);
	return e;
}


vec3 ray_sample(vec2 pos)
{
	vec3 emissive = vec3(0);
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
		float angle = TWO_PI * (rangle[i] + noise) / (SAMPLE * ITERATION);
		//float angle = (i + noise);
		//float a =  (i + texture2D(texture1, gl_FragCoord.xy / noise_size).x);
		//float a =  2*3.1415926 *(i + o*1 + 0*  LFSR_Rand_Gen(pos)) / 64;

		// push sample ray to stack for ray marching
		ray_buffer[0] = ray(pos, vec2(cos(angle), sin(angle)), vec3(1), RAY_DEPTH);
		emissive += march();
	}
	return emissive / (SAMPLE * ITERATION);
}

void main()
{
	vec2 frag_coord = gl_FragCoord.xy / min(viewport_size.x, viewport_size.y);
	vec3 color = ray_sample(frag_coord);		
	frag_color = texture2D(frame_canvas, tex_coords) + vec4(color.xyz, 1);
}