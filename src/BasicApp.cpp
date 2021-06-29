#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/app/Window.h"
#if ! defined( CINDDER_GL_ES )
	#include "cinder/CinderImGui.h"
#endif
#include <memory>
//rapidjson
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
#include <iostream>
#include <fstream>
#include <string>
#include "cinder/gl/Fbo.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

using namespace ci;
using namespace rapidjson;

namespace ImguiWrp {
	bool ColorPick(char* label, ci::Color& pro) {
		float* col = (float*)&(pro);
		ImGui::ColorEdit3(label, col);
		pro = ci::Color(col[0], col[1], col[2]);
		return true;
	}


}
using namespace ci::app;
namespace sh {
	class Shape {
	public:
		vec2 loc;
		ci::Color color;
		enum Type { none, circle, square, rectangle };
		Type typ = Type::none;
		bool rogue = false;
		Shape(vec2 loci, ci::Color coli) {
			loc = loci;
			color = coli;
		}
		virtual void Draw() {};
		virtual bool Inside(vec2 pos) {
			return false;
		};
		virtual void Update(vec2 windowsize) {};
		virtual char* GetText() {
			return "Object";
		}
		std::map<Type, char*> enumstr = {
			{Type::none, "none"},
			{Type::circle, "circle"},
			{Type::square, "square"},
			{Type::rectangle, "rectangle"}
		};
		void showProperties() {
			if (rogue) {
				ImGui::Text("Type: %s", enumstr[typ]);
				ci::Color tmp = color;
				ImguiWrp::ColorPick("Color", color);
				color = tmp;
				vec2 tmp1 = loc;
				ImGui::InputFloat2("Location", (float*)&loc);
				loc = tmp1;
			}
			else {
				ImGui::Text("Type: %s", enumstr[typ]);
				ImguiWrp::ColorPick("Color", color);
				ImGui::InputFloat2("Location", (float*)&loc);
			}

		}

		virtual void showExtendedProperties() {

		}

	};
	class Circle : public Shape {
	public:
		int radius;
		vec2 dir;
		//Type typ = Type::circle;
		Circle(vec2 loci, ci::Color coli, int radi) : Shape(loci, coli) {
			radius = radi;
			dir = { randFloat() , randFloat() };
			typ = Type::circle;
		}
		void Draw() {
			gl::color(color);
			gl::drawSolidCircle(loc, radius);
		}
		bool Inside(vec2 pos) {
			return (loc.x - pos.x) * (loc.x - pos.x) + (loc.y - pos.y) * (loc.y - pos.y) < radius * radius;
		}
		void Update(vec2 windowsize) {
			loc += dir;
			if (loc.x - radius < 0 || loc.x + radius > windowsize.x) {
				dir.x = -dir.x;
			}
			if (loc.y - radius < 0 || loc.y + radius > windowsize.y) {
				dir.y = -dir.y;
			}
		}
		void showExtendedProperties() {
			if (rogue) {
				int tmp = radius;
				ImGui::InputInt("radius", &radius);
				radius = tmp;
				vec2 tmp1 = dir;
				ImGui::DragFloat2("Direction", (float*)&dir);
				dir = tmp1;
			}
			else {
				ImGui::InputInt("radius", &radius);
				ImGui::DragFloat2("Direction", (float*)&dir);
			}
		}
	};
	
	class Rectangle : public Shape {
	public:
		int w;
		int h;
		vec2 dir;
		float rot;
		Rectangle(vec2 loci, ci::Color coli, int wi, int hi, float roti) : Shape(loci, coli) {
			w = wi;
			h = hi;
			rot = roti;
			dir = { randFloat() , randFloat() };
			typ = Type::square;
		}
		void Draw() {
			gl::color(color);
			gl::rotate(rot);
			gl::drawSolidRect(Rectf(), loc, vec2(loc[0] + w, loc[1] + h));
		}
		bool Inside(vec2 pos) {
			return (loc[0] < pos[0]) && (loc[0] + w > pos[0]) && (loc[1] < pos[1]) && (loc[1]+h > pos[1]);
		}
		void Update(vec2 windowsize) {
			loc += dir;
			if (loc.x < 0 || loc.x + w > windowsize.x) {
				dir.x = -dir.x;
			}
			if (loc.y < 0 || loc.y + h > windowsize.y) {
				dir.y = -dir.y;
			}
		}
		virtual void showExtendedProperties() {
			if (rogue) {
				int tmpw = w;
				ImGui::InputInt("width", &w);
				w = tmpw;
				int tmph = h;
				ImGui::InputInt("width", &h);
				h = tmph;
				vec2 tmp1 = dir;
				ImGui::DragFloat2("Direction", (float*)&dir);
				dir = tmp1;
			}
			else {
				ImGui::InputInt("width", &w);
				ImGui::InputInt("hight", &h);
				ImGui::DragFloat2("Direction", (float*)&dir);
			}
		}
	};
	class Square : public Rectangle {
	public:
		Square(vec2 loci, ci::Color coli, int sizi, float roti) : Rectangle (loci, coli, sizi, sizi, roti) {
			typ = Type::square;
		}
		virtual void showExtendedProperties() {
			if (rogue) {
				int tmpw = w;
				ImGui::InputInt("size", &w);
				w = tmpw;
				vec2 tmp1 = dir;
				ImGui::DragFloat2("Direction", (float*)&dir);
				dir = tmp1;
			}
			else {
				ImGui::InputInt("size", &w);
				ImGui::DragFloat2("Direction", (float*)&dir);
			}
		}
	};
}
namespace jsonfy {
	Document d;
	Value savelocation(vec2 loc) {
		Value locobject;
		locobject.SetObject();
		locobject.AddMember("x", Value().SetFloat(loc[0]), d.GetAllocator());
		locobject.AddMember("y", Value().SetFloat(loc[1]), d.GetAllocator());
		return locobject;
	}
	vec2 loadloacation(Value& loc) {
		return vec2(loc['x'].GetFloat(), loc['y'].GetFloat());
	}
	Value saveobject(sh::Shape& obj) {
		Value shapeObject;
		shapeObject.SetObject();
		shapeObject.AddMember("type", Value((obj.enumstr[obj.typ]), d.GetAllocator()), d.GetAllocator());

		char hexcol[16];
		snprintf(hexcol, sizeof hexcol, "0x%02x%02x%02x", (int)(((float*)&(obj.color))[0] * 255), (int)(((float*)&(obj.color))[1] * 255), (int)(((float*)&(obj.color))[2] * 255));
		shapeObject.AddMember("color", Value(hexcol, d.GetAllocator()), d.GetAllocator());
		shapeObject.AddMember("location", savelocation(obj.loc), d.GetAllocator());
		shapeObject.AddMember("rogue", obj.rogue, d.GetAllocator());
		if (obj.typ == obj.Type::circle) {
			sh::Circle* cir = (sh::Circle*)&(obj);
			shapeObject.AddMember("radius", Value().SetInt(cir->radius), d.GetAllocator());
			shapeObject.AddMember("direction", savelocation(cir->dir), d.GetAllocator());
		}
		if (obj.typ == obj.Type::rectangle) {
			sh::Rectangle* cir = (sh::Rectangle*)&(obj);
			shapeObject.AddMember("width", Value().SetInt(cir->w), d.GetAllocator());
			shapeObject.AddMember("height", Value().SetInt(cir->h), d.GetAllocator());
			shapeObject.AddMember("rotation", Value().SetFloat(cir->rot), d.GetAllocator());
			shapeObject.AddMember("direction", savelocation(cir->dir), d.GetAllocator());
		}
		if (obj.typ == obj.Type::square) {
			sh::Square* cir = (sh::Square*)&(obj);
			shapeObject.AddMember("size", Value().SetInt(cir->w), d.GetAllocator());
			shapeObject.AddMember("rotation", Value().SetFloat(cir->rot), d.GetAllocator());
			shapeObject.AddMember("direction", savelocation(cir->dir), d.GetAllocator());
		}
		return shapeObject;
	};
	void loadObject(Value& shapeObject, std::vector<std::unique_ptr<sh::Shape>>& objs) {
		//shapeObject.SetObject();
		assert(shapeObject.HasMember("type"));
		assert(shapeObject.HasMember("location"));
		if ((shapeObject["type"]) == "circle") {
			objs.push_back(std::unique_ptr<sh::Circle>(new sh::Circle(loadloacation(shapeObject["location"]), ci::Color().hex((uint32_t)std::stoi(shapeObject["color"].GetString())), shapeObject["radious"].GetInt())));
		}
	}
	
	std::string save(std::vector<std::unique_ptr<sh::Shape>>& objs) {
		Value objArray(kArrayType);
		objArray.SetArray();
		Document::AllocatorType& allocator = d.GetAllocator();
		for (int i = 0; i < objs.size(); i++) {
			objArray.PushBack(Value(saveobject(*objs[i]), allocator), allocator);
		}
		//d["objects"] = objArray;
		//assert(objArray.IsObject());
		d.CopyFrom(Value(objArray, d.GetAllocator()), d.GetAllocator());
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		d.Accept(writer);
		return buffer.GetString();
	}
	void loadObjects(char * jsonstring, std::vector<std::unique_ptr<sh::Shape>>& objects) {
		d.Parse(jsonstring);
		//d.SetArray();
		for (auto& v : d.GetArray()) {
			assert(v.HasMember("type"));
			loadObject(v, objects);
		}
	}
	
}
class BasicApp : public App {
  public:
	// Cinder will call 'mouseDrag' when the user moves the mouse while holding one of its buttons.
	// See also: mouseMove, mouseDown, mouseUp and mouseWheel.
	void mouseDown( MouseEvent event ) override;
	void fileDrop(FileDropEvent event) override;
	// Cinder will call 'keyDown' when the user presses a key on the keyboard.
	// See also: keyUp.
	void keyDown( KeyEvent event ) override;

	// Cinder will call 'draw' each time the contents of the window need to be redrawn.
	void draw() override;
	void update() override;
	void setup() override;
  private:
	// This will maintain a list of points which we will draw line segments between
	std::vector<std::unique_ptr<sh::Shape>> objects;
	public:
	const int pos_change = 4;
	gl::Texture2dRef texture;
	gl::FboRef fbo;
	std::vector<Surface> pictures;
	std::vector<std::string> picfilenames;
};

void prepareSettings( BasicApp::Settings* settings )
{
	settings->setMultiTouchEnabled( false );
}
void BasicApp::setup() {
	ImGui::Initialize();
	Surface mySurface(640, 480, true);
	mySurface = loadImage("texture5.jpg");
	texture = gl::Texture2d::create(mySurface);
	fbo = gl::Fbo::create(640, 480);
}
void BasicApp::mouseDown( MouseEvent event )
{
	// Store the current mouse position in the list.
	//mPoints.push_back( event.getPos() );
	if (event.isLeft()) {
		objects.push_back(std::unique_ptr<sh::Circle>(new sh::Circle(event.getPos(), Color(randFloat(), randFloat(), randFloat()), randInt(10, 100))));
	}
	else if(event.isRight()) {
		int i = objects.size()-1;
		while (i >= 0 && !objects[i]->Inside(event.getPos())) {
			i--;
		}
		if (i >= 0) {
			objects.erase(objects.begin() + i);
		}
	}
}
void BasicApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		// Toggle full screen when the user presses the 'f' key.
		setFullScreen( ! isFullScreen() );
	}
	else if (event.getChar() == 'w' && objects.size() > NULL) {
		objects.back()->loc.y -= pos_change;
	}
	else if (event.getChar() == 's' && objects.size() > NULL) {
		objects.back()->loc.y += pos_change;
	}
	else if (event.getChar() == 'd' && objects.size() > NULL) {
		objects.back()->loc.x += pos_change;
	}
	else if (event.getChar() == 'a' && objects.size() > NULL) {
		objects.back()->loc.x -= pos_change;
	}
	else if( event.getCode() == KeyEvent::KEY_SPACE ) {
		// Clear the list of points when the user presses the space bar.
		//mPoints.clear();
	}
	else if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
		// Exit full screen, or quit the application, when the user presses the ESC key.
		if( isFullScreen() )
			setFullScreen( false );
		else
			quit();
	}
}
void BasicApp::fileDrop(FileDropEvent event) {
	for (auto& p : event.getFiles()) {
		Surface mySurface(640, 480, true);
		mySurface = Surface(loadImage(p));
		pictures.push_back(mySurface);
		picfilenames.push_back(p.filename().string());
	}
}
void BasicApp::draw()
{
	// Clear the contents of the window. This call will clear
	// both the color and depth buffers. 
	gl::clear(Color::gray(0.1f));
	gl::color(Colorf::white());
	fbo->bindFramebuffer();
	if(pictures.size() > 0){
		int slice = 640 / pictures.size();
		for (int i = 0; i < pictures.size(); ++i) {
			Rectf destRect = Rectf(slice*i, 0, slice * (i+1), 480);
			//destRect.offset(vec2(slice * i, 0));
			gl::draw(gl::Texture2d::create(pictures[i].clone(Area(destRect))), destRect);
		}
		//gl::draw(texture, Area(destRect.getCenteredFit(texture->getBounds(), true)), destRect);
	}
	fbo->unbindFramebuffer();

	gl::draw(fbo->getColorTexture(), getWindowBounds());
	for (auto& ele : objects) {
		ele->Draw();
	}
	
}
bool VectorOfStringGetter(void* data, int n, const char** out_text)
{
	const std::vector<std::unique_ptr<sh::Shape>>*v = (std::vector<std::unique_ptr<sh::Shape>>*)data;
	*out_text = (*v)[n]->GetText();
	return true;
}
void Setup(sh::Shape) {

}
void BasicApp::update() {
	
	//ImGui::ShowDemoWindow();
	static int listbox_item_current = -1;
	{
		ImGui::Begin("Objects");
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save")) {
					std::ofstream out("project.json");
					out << jsonfy::save(objects);
					out.close();
				}
				if (ImGui::MenuItem("Open")) {
					std::ifstream t("project.json");
					std::string jsonstring((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
					//TODO
					//jsonfy::loadObjects((char* )jsonstring.c_str(), objects);
				}

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		ImGui::ListBox("", &listbox_item_current, VectorOfStringGetter, static_cast<void*>(&objects), objects.size());
		ImGui::End();
	}
	{
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
			listbox_item_current = -1;
		}
		ImGui::Begin("Properties");
		
		if (listbox_item_current != -1 && listbox_item_current < objects.size()) {
			
			sh::Shape &pro = *(objects[listbox_item_current]); 
			pro.showProperties();
			pro.showExtendedProperties();
			/*
			ImGui::Text("Type: %s", pro.enumstr[pro.typ]);
			ImguiWrp::ColorPick("MyColor##1", pro.color);
			*/

		}
		float col[3];
		
		ImGui::End();
		ImGui::Begin("Harmonica");
		{
			for (int i = 0; i < pictures.size(); i++) {
				ImGui::Text(picfilenames[i].c_str());
			}
		}
		ImGui::End();
	};
	for (auto& ele : objects) {
		ele->Update(getWindowSize());
	}
}
// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicApp, RendererGl, prepareSettings )
