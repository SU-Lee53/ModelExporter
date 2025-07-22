#include "stdafx.h"
#include "AssimpImporter.h"

int main()
{
	std::unique_ptr<AssimpImporter> pImporter;
	pImporter = std::make_unique<AssimpImporter>();
	
	if (pImporter) {
		std::println("Importer Initialized");
	}
	
	while (true) {
		std::cout << "Input model path : ";
		std::string path = "../../Models/Sporty Granny.fbx";
		//std::cin >> path;
	
		if (!pImporter->LoadModelFromPath(path)) {
			continue;
		}
	
		std::cout << path << " : file read success" << std::endl;
		while (true) {
			std::cout << "1. Show Scene" << std::endl;
			std::cout << "2. Show Every Nodes" << std::endl;
	
			int n;
			std::cin >> n;
	
			switch (n)
			{
			case 1:
				pImporter->ShowSceneAttribute();
				break;
	
			case 2:
				pImporter->ShowNodeAll();
				break;
	
			default:
				break;
			}
	
		}
	
	}






}