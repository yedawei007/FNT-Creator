//
//  FontCreator.cpp
//  FontCreatorCpp
//
//  Created by yedawei on 1/6/15.
//
//

#include "FontCreator.h"
#include <fstream>
#include <utility>
#include <codecvt>
#include <locale>
namespace game {
namespace tools {
    USING_NS_CC;
    using namespace std;
    
    // copy from http://www.cnblogs.com/mrblue/p/3407654.html
    static std::list<std::string> split_utf8_string(const std::string& text)
    {
        std::list<std::string> temp;
        do
        {
            if (text.length() <= 0)
                break;
            std::string::const_iterator begin = text.begin();
            std::string::const_iterator end   = text.end();
            while (begin != end)
            {
                unsigned char c = *begin;
                int n = 0;
                if ((c & 0x80) == 0)
                    n = 1;
                else if ((c & 0xE0) == 0xC0)
                    n = 2;
                else if ((c & 0xF0) == 0xE0)
                    n = 3;
                else if ((c & 0xF8) == 0xF0)
                    n = 4;
                else if ((c & 0xFC) == 0xF8)
                    n = 5;
                else if ((c & 0xFE) == 0xFC)
                    n = 6;
                else
                    break;
                if (end - begin < n)
                    break;
                std::string substring;
                substring += *begin;
                bool isError = false;
                for (int i=1; i<n; ++i)
                {
                    if ((begin[i] & 0xC0) != 0x80)
                    {
                        isError = true;
                        break;
                    }
                    substring += begin[i];
                }
                if (isError)
                    break;
                temp.push_back(substring);
                begin += n;
            }
        } 
        while (false);
        return temp;
    }
    
    struct CharInfo{
        string c;
        int x;
        int y;
        int width;
        int height;
        int xoffset;
        int yoffset;
        int xadvance;
        int page;
        int chnl;
    } ;

    struct FNTStruct {
        Layer * layer;
        string fontName;
        int size;
        int lineHeight;
        std::vector<CharInfo> chars;
        FNTStruct()
        : chars()
        , layer(nullptr)
        {
            
        }
    } ;
    
    static Size sizeOfFontDef(const FontDefinition & def) {
        auto label = LabelTTF::createWithFontDefinition("建", const_cast<FontDefinition & >(def));
        Size sz = label->getContentSize();
        return sz;
    }
    
    static int trim(int value) {
        for (int i = 1; i < 32; i ++) {
            int v = 1 << i;
            if (value < v) {
                return v;
            }
        }
        return 1;
    }
    
    static int sqr(int value) {
        int r = (int)sqrtf(value);
        return trim(r);
    }
    
    static FNTStruct createFNTStruct(const std::string & text, const FontDefinition & def) {
        std::list<std::string> l = split_utf8_string(text);
        std::set<std::string> s(l.begin(), l.end());
        
        Size sz = sizeOfFontDef(def);
        int maxWidth = s.size() * sz.width;
        int minW = sqr(maxWidth * sz.height);
        
        int x = 0;
        int y = 0;
        
        Layer * layer = Layer::create();
        layer->setAnchorPoint(Vec2(0, 0));
        layer->ignoreAnchorPointForPosition(false);
        layer->setContentSize(Size(1024, 1024));
        FNTStruct fnt;
        fnt.fontName = def._fontName;
        fnt.size = def._fontSize;
        fnt.lineHeight = sz.height;
        int lineMaxHeight = 0;
        auto nextLine = [&]() {
            x = 0;
            y = y + lineMaxHeight + 2;
            lineMaxHeight = 0;
        };

        for (auto c : s) {
            auto label = LabelTTF::createWithFontDefinition(c, const_cast<FontDefinition & >(def));
            Size csz = label->getContentSize();
            if (x + csz.width > minW) {
                nextLine();
            }
            if (csz.height > lineMaxHeight) {
                lineMaxHeight = csz.height;
            }
            label->setPosition(x, y);
            label->setAnchorPoint(Vec2(0, 0));
            layer->addChild(label);
            CharInfo info;
            info.c = c;
            info.x = x;
            info.y = y;
            info.width = csz.width;
            info.height = csz.height;
            info.xoffset = 0;
            info.yoffset = 0;
            info.xadvance = csz.width;
            if (def._shadow._shadowEnabled) {
                if (def._shadow._shadowOffset.width < 0) {
                    info.xoffset = -def._shadow._shadowOffset.width;
                }
                info.xadvance = info.xadvance - fabs(def._shadow._shadowOffset.width);
                if (def._shadow._shadowOffset.height > 0) {
                    info.yoffset = def._shadow._shadowOffset.height;
                }
            }
            if (def._stroke._strokeEnabled) {
                info.xoffset = info.xoffset + (int)def._stroke._strokeSize;
                info.xadvance = info.xadvance - (int)def._stroke._strokeSize;
            }
            info.page = 0;
            info.chnl = 0;
            fnt.chars.push_back(info);
            x = x + csz.width + 2;
            if (x > minW) {
                nextLine();
            }
        }
        int lwidth = minW;
        int lheight = 0;
        if (x == 0) {
            lheight = y - 2;
        } else {
            lheight = y + lineMaxHeight;
        }
        lheight = trim(lheight);
        layer->setContentSize(Size(lwidth, lheight));
        fnt.layer = layer;
        return fnt;
    }
    
    static void writeToFile(const string & name, const FNTStruct & fnt) {
        string path = FileUtils::getInstance()->getWritablePath() + name;
        CCLOG("writeTo: %s%s", path.c_str(), ".fnt");
        Size sz = fnt.layer->getContentSize();
        std::ofstream fs(path + ".fnt", std::ios::binary);
        fs << "info face=\"" << fnt.fontName << "\" size=" << fnt.size << " bold=0 italic=0 charset=\"\" unicode=0 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1\n";
        fs << "common lineHeight=" << fnt.lineHeight << " base=26 scakeW=" << sz.width << " scaleH=" << sz.height << " pages=1 packed=0\n";
        fs << "page id=0 file=\"" << name << ".png\"\n";
        std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
        for (auto info : fnt.chars) {
            std::wstring ws = utf8_conv.from_bytes(info.c);
            fs << "char id=" << ((int)ws.at(0)) << "   x=" << info.x << "   y="<< (sz.height - info.y - info.height) << "   width="<<info.width
            << "   height=" << info.height << "   xoffset=" << info.xoffset << "   yoffset="<<info.yoffset
            << "   xadvance="<<info.xadvance << "   page="<<info.page << "   chnl=" << info.chnl << endl;
        }
        fs.flush();
        fs.close();
        auto texture = RenderTexture::create(sz.width, sz.height);
        texture->retain();
        texture->beginWithClear(0, 0, 0, 0);
        fnt.layer->setPosition(0, 0);
        fnt.layer->visit();
        texture->end();
        texture->saveToFile(name + ".png", Image::Format::PNG);
        texture->release();
    }
    
    static int asInt(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            return (*it).second.asInt();
        } else {
            return 0;
        }
    }
    
    static string asString(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            return (*it).second.asString();
        } else {
            return string("");
        }
    }
    
    static float asFloat(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            return (*it).second.asFloat();
        } else {
            return 0.0f;
        }
    }
    
    static bool asBool(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            return (*it).second.asBool();
        } else {
            return false;
        }
    }
    
    static Color3B asColor3B(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            ValueMap cvm = (*it).second.asValueMap();
            int r = asInt(cvm, "r");
            int g = asInt(cvm, "g");
            int b = asInt(cvm, "b");
            return Color3B(r, g, b);
        } else {
            return Color3B::WHITE;
        }
    }
    
    static Size asSize(const ValueMap& vm, string key) {
        ValueMap::const_iterator it = vm.find(key);
        if (it != vm.end()) {
            ValueMap svm = (*it).second.asValueMap();
            int width = asInt(svm, "width");
            int height = asInt(svm, "height");
            return Size(width, height);
        } else {
            return Size::ZERO;
        }
    }
    
    static FontDefinition FontDefinitionFromValueMap(const ValueMap& vm) {
        FontDefinition fontDef;
        fontDef._fontName = asString(vm, "fontName");
        fontDef._fontSize = asInt(vm,"fontSize");
        fontDef._fontFillColor = asColor3B(vm, "fontColor");
        fontDef._shadow._shadowEnabled = asBool(vm,"shadowEnabled");
        if (fontDef._shadow._shadowEnabled) {
            fontDef._shadow._shadowBlur = asFloat(vm,"shadowBlur");
            fontDef._shadow._shadowOffset = asSize(vm, "shadowOffset");
            fontDef._shadow._shadowOpacity = asFloat(vm,"shadowOpacity");
        }
        fontDef._stroke._strokeEnabled = asBool(vm,"strokeEnabled");
        if (fontDef._stroke._strokeEnabled) {
            fontDef._stroke._strokeSize = asInt(vm,"strokeSize");
            fontDef._stroke._strokeColor = asColor3B(vm, "stockColor");
        }
        return fontDef;
    }
    
    static string readAll(string & path) {
        std::ifstream t(path);
        return std::string((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
    }
    
    void createFNTFiles(const string & configFile) {
        auto fileUtils = FileUtils::getInstance();
        auto fontDefines = fileUtils->getValueVectorFromFile(configFile);
        for (auto value : fontDefines) {
            auto vm = value.asValueMap();
            auto fontDef = FontDefinitionFromValueMap(vm);
            auto name = asString(vm, "name");
            string path(fileUtils->fullPathForFilename(name + ".txt"));
            auto text = readAll(path);
            auto fnt = createFNTStruct(text, fontDef);
            writeToFile(name, fnt);
        }
    }
    
    Layer * createFontCreator() {
        createFNTFiles("sampleFontDefines.plist");
        auto layer = Layer::create();
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto origin = Director::getInstance()->getVisibleOrigin();
        auto colorLayer = LayerColor::create(Color4B(255, 255, 255, 255), visibleSize.width, visibleSize.height);
        layer->addChild(colorLayer);
        FontDefinition fontDef;
        fontDef._fontName = "Arial";
        fontDef._fontSize = 100;
        fontDef._fontFillColor = Color3B(255, 0, 0);
        fontDef._shadow._shadowEnabled = true;
        fontDef._shadow._shadowBlur = 0.5;
        fontDef._shadow._shadowOffset = Size(4, 0);
        fontDef._shadow._shadowOpacity = 1.0f;
        fontDef._stroke._strokeEnabled = true;
        fontDef._stroke._strokeSize = 4;
        fontDef._stroke._strokeColor = Color3B(0, 0, 255);
        string text = "Task Complete";
        auto label = LabelTTF::createWithFontDefinition(text, fontDef);
        label->setPosition(visibleSize.width/2, visibleSize.height/2);
        layer->addChild(label);
        return layer;
    }
    
};

};