//
//  FontCreator.h
//  FontCreatorCpp
//
//  Created by yedawei on 1/6/15.
//
//

#ifndef __FontCreatorCpp__FontCreator__
#define __FontCreatorCpp__FontCreator__
#include "cocos2d.h"
namespace game {
namespace tools {
    void createFNTFiles(const std::string & configFile);
    cocos2d::Layer * createFontCreator();
};
};
#endif /* defined(__FontCreatorCpp__FontCreator__) */
