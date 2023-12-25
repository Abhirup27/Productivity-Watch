#include <iostream>

void convert(const char** source, const int size, const char* target[]) {
    for (int i = 0; i < size; ++i) {
        target[i] = source[i];
    }
}

int main() {
    const char* strings[] = {"Hello", "World", "Example"};
    const int size = sizeof(strings) / sizeof(strings[0]);

    // Create an array of pointers to constant characters
    const char* convertedArray[size];

    // Convert const char** to const char* []
    convert(strings, size, convertedArray);

    // Print the converted array
    for (int i = 0; i < size; ++i) {
        std::cout << convertedArray[i] << std::endl;
    }

    return 0;
}
