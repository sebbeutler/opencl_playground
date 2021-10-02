
typedef struct Particle
{
    float x;
    float y;
    float vX;
    float vY;
} Particle;

kernel void main(global Particle* particles, double deltaTime, int mx, int my)
{
    int id = get_global_id(0);
    Particle* p = &particles[id];
    float nx = mx - p->x;
    float ny = my - p->y;
    float magnitude = sqrt(nx*nx + ny*ny);
    nx /= magnitude;
    ny /= magnitude;

    p->vX += nx * 5 * deltaTime;
    p->vY += ny * 5 * deltaTime;
    

    p->x += p->vX;
    p->y += p->vY;
}