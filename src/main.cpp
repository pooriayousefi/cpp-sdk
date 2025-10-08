#include <iostream>
#include <exception>
#include <cstdlib>

// entry-point
int main()
{
    try
    {
        // start here ...
        
        return EXIT_SUCCESS;
    }
    catch (const std::exception& xxx)
    {
        std::cerr << "Error: " << xxx.what() << std::endl;
        return EXIT_FAILURE;
    }
}
