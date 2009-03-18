# GravitySolver

Implements a potentially useful gravity solver utility class.

Supports Directional, Central, Cylindrical, and Standard gravity types.

e.g.:

    GravitySolver solver(-9.8); // standard gravity
    GravitySolver::Result result; // Resultant forces
    
    // Grab forces up in the sky
    solver.getForces(Point3F(0.0, 0.0, 10.0), result);
    
    // What you get
    Point3F upDirection = solver.rDirection;
    float arbitraryMaxVelocity = solver.rVelocity;
    float timeTillTerminalVelocity = arbitraryMaxVelocity / solver.rAcceleration;
    
Disclaimer: may or may not work correctly. So don't rely on it for your safety parachute deployment system.
