#include "OctaneEngine.hpp"

int main()
{
    OctaneEngine file = OctaneEngine("https://file-examples.com/wp-content/uploads/2017/04/file_example_MP4_1920_18MG.mp4", "snake.mp4");

    file.get_url();
    file.get_filesize();
    file.get_partsize();

    file.download_file();
    return 0;
}
