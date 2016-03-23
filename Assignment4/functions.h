void loadAllObjects(string filename)
{
	ifstream file(filename);
	string line;
	ifstream line2;
	string data;
	vector<string> words;
	int i = 0;

	while (getline(file, line, ' '))
	{
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '}'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '{'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '#'), line.end());
		words.push_back(line);
	}

	while (i < words.size())
	{
		if (!words.at(i).find("light"))
		{
			light l;
			l.position = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			l.intensity = stof(words.at(i + 7));
			lights.push_back(l);
		}

		if (!words.at(i).find("sphere"))
		{
			sphere s;
			s.center = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			s.radius = stof(words.at(i + 7));
			s.color = vec3(stof(words.at(i + 9)), stof(words.at(i + 10)), stof(words.at(i + 11)));
			spheres.push_back(s);
		}

		if (!words.at(i).find("triangle"))
		{
			triangle t;
			t.P0 = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			t.P1 = vec3(stof(words.at(i + 7)), stof(words.at(i + 8)), stof(words.at(i + 9)));
			t.P2 = vec3(stof(words.at(i + 11)), stof(words.at(i + 12)), stof(words.at(i + 13)));
			t.color = vec3(stof(words.at(i + 15)), stof(words.at(i + 16)), stof(words.at(i + 17)));
			triangles.push_back(t);
		}

		if (!words.at(i).find("plane"))
		{
			plane p;
			p.normal = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			p.position = vec3(stof(words.at(i + 7)), stof(words.at(i + 8)), stof(words.at(i + 9)));
			p.color = vec3(stof(words.at(i + 11)), stof(words.at(i + 12)), stof(words.at(i + 13)));
			planes.push_back(p);
		}
		i++;
	}
}

float max(float a, float b)
{
	if (a >= b)
		return a;
	else
		return b;
}

vec3 Phong(light light, vec3 point, vec3 normal, vec3 color, ray r, bool draw)
{
	vec3 l = normalize(light.position - point);
	vec3 v = normalize(r.origin - point);
	vec3 n = normalize(normal);

	vec3 h = normalize(v + l);

	vec3 ka = color;
	vec3 kd = ka;
	vec3 ks = vec3(0.7, 0.7, 0.7);
	float Ia = 0.2;
	float I = light.intensity;
	int exp = 16;

	vec3 ambient = ka * Ia;
	vec3 diffuse = kd * I * max(0, dot(n, l));
	vec3 specular = ks * I * pow(max(0, dot(n, h)), exp);
	vec3 L = ambient + diffuse + specular;
	if (draw == false)
		return ambient + diffuse;
	else
		return L;
}

vec4 intersectSphere(ray r, sphere sphere, light light)
{
	//calculate quadratic variables
	float a = dot(r.direction, r.direction);
	float b = (2 * r.direction.x * (r.origin.x - sphere.center.x)) +
		(2 * r.direction.y * (r.origin.y - sphere.center.y)) +
		(2 * r.direction.z * (r.origin.z - sphere.center.z));
	float c = dot(sphere.center, sphere.center) + dot(r.origin, r.origin) + (-2 * dot(sphere.center, r.origin)) - pow(sphere.radius, 2);

	float determ = pow(b, 2) - 4 * a*c;	//calculates determinant

	if (determ < 0)						//if determ < 0 then no intersection
		return vec4(NULL, NULL, NULL, NULL);
	else
	{
		float t1 = (-b + determ) / (2 * a);
		float t2 = (-b - determ) / (2 * a);
		float t;

		if (t1 <= t2){ t = t1; }
		else{ t = t2; }

		vec3 x = r.origin + (t*r.direction);
		vec3 n = normalize(x - sphere.center);

		ray newRay;
		newRay.direction = light.position - x;
		newRay.origin = x;
		vec3 color;

		if (anyIntersect(newRay))
		{
			vec3 color = vec3(0, 0, 0);
		}
		else
		{
			vec3 color = Phong(light, x, n, sphere.color, r, true);
		}

		return vec4(color, t);
	}

}

vec4 intersectPlane(ray ray, plane plane, light light)
{
	float para = dot(plane.normal, ray.direction);
	if (para != 0)
	{
		vec3 ppco = plane.position - ray.origin; //planePosition -cameraOrigin
		float t = dot(ppco, plane.normal) / para;

		if (t < 0)
			return vec4(NULL, NULL, NULL, NULL);

		vec3 x = ray.origin + (t*ray.direction);
		vec3 color = Phong(light, x, plane.normal, plane.color, ray, false);

		return vec4(color, t);
	}
	else
	{
		return vec4(NULL, NULL, NULL, NULL);
	}
}

vec4 intersectTriangle(ray ray, triangle tri, light light)
{
	//compute normal vector of plane which triangle resides
	vec3 P1P0 = tri.P1 - tri.P0;
	vec3 P2P0 = tri.P2 - tri.P0;
	vec3 P2P1 = tri.P2 - tri.P1;
	vec3 P0P2 = tri.P0 - tri.P2;
	vec3 normal = normalize(cross(P1P0, P2P0));

	plane p;
	p.normal = normal;
	p.position = tri.P0;

	vec4 plane = intersectPlane(ray, p, light);

	vec3 x = ray.origin + (plane.w*ray.direction); //plane.w is t

	float a = dot(cross(P1P0, (x - tri.P0)), normal);
	float b = dot(cross(P2P1, (x - tri.P1)), normal);
	float c = dot(cross(P0P2, (x - tri.P2)), normal);

	if (a >= -0.001 && b >= -0.001 && c >= -0.001)
	{
		vec3 color = Phong(light, x, normal, tri.color, ray, false);
		return vec4(color, plane.w);
	}
	else
	{
		return vec4(NULL, NULL, NULL, NULL);
	}
}

bool anyIntersect(ray r)
{
	bool inShadow = false;
	vec4 intersect;

	for (int i = 0; i < spheres.size(); i++)
	{
		intersect = intersectSphere(r, spheres.at(i), lights.at(0));
		if (intersect.w != NULL)
			inShadow = true;
	}

	//check intersect with all planes
	for (int i = 0; i < planes.size(); i++)
	{
		intersect = intersectPlane(r, planes.at(i), lights.at(0));
		if (intersect.w != NULL)
			inShadow = true;

	}

	//check intersect with all triangles
	for (int i = 0; i < triangles.size(); i++)
	{
		intersect = intersectTriangle(r, triangles.at(i), lights.at(0));
		if (intersect.w != NULL)
			inShadow = true;
	}
	return inShadow;
}