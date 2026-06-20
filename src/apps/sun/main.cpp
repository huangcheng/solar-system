#include "BodyApp.h"
#include "BodyConfig.h"

int main(int argc, char *argv[]) {
    return bodyapp::runBodyApp(argc, argv, BodyConfig::sun());
}
