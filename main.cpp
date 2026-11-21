#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdlib>

using namespace std;

const float PI = 3.14159265f;
const int SIDEBAR = 310;

struct Point { float x, z; };
struct Building { float x, z, w, d, h, r, g, b; };
struct Tree { float x, z, height, radius; bool proposed; };
struct Shadow { vector<Point> p; };

vector<Building> buildings;
vector<Tree> trees;
vector<Shadow> shadows;

int winW = 1400, winH = 850;
float simHour = 9.0f;
float sunX = -18, sunY = 15, sunZ = -8;
float cameraAngle = 42, cameraHeight = 24, cameraDistance = 43;
float cursorX = 4.2f, cursorZ = 0;
float selectedTreeRadius = 1.8f, selectedTreeHeight = 3.4f;
float shadePct = 0, shadedArea = 0, exposedArea = 0;
int shadedSamples = 0, totalSamples = 0;
float baselinePct = -1;
float bestGain = 0;
Point bestSpot = {0,0};
bool showHeatmap = true, showShadows = true, showProposed = true;
bool topView = false, autoRotate = false, showGrid = true;
bool draggingTime = false;
string statusMessage = "Ready - select a tool or adjust simulation time";

void setColor(float r,float g,float b){ glColor3f(r,g,b); }

void initCity() {
    buildings.clear(); trees.clear();
    buildings.push_back({-9,-10,5,7,12,.65f,.68f,.75f});
    buildings.push_back({-9,0,5,7,18,.75f,.65f,.55f});
    buildings.push_back({-9,10,5,7,14,.55f,.68f,.65f});
    buildings.push_back({9,-10,5,7,16,.65f,.60f,.72f});
    buildings.push_back({9,0,5,7,10,.72f,.68f,.55f});
    buildings.push_back({9,10,5,7,20,.58f,.68f,.78f});
    for(int z=-15;z<=15;z+=10){
        trees.push_back({-4.2f,(float)z,3.4f,1.8f,false});
        trees.push_back({ 4.2f,(float)z,3.4f,1.8f,false});
    }
}

void updateSun() {
    float t=(simHour-6.0f)/12.0f;
    if(t<0)t=0; if(t>1)t=1;
    sunX=-22.0f+44.0f*t;
    sunY=3.5f+22.0f*sin(PI*t);
    sunZ=-7.0f+14.0f*t;
}

Point projectShadow(float x,float y,float z) {
    Point p;
    if(sunY<=y+0.01f){p.x=x;p.z=z;return p;}
    float t=sunY/(sunY-y);
    p.x=sunX+t*(x-sunX);
    p.z=sunZ+t*(z-sunZ);
    return p;
}

float crossP(Point a,Point b,Point c){
    return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);
}
bool pointLess(Point a,Point b){if(a.x==b.x)return a.z<b.z;return a.x<b.x;}

vector<Point> hull(vector<Point> p){
    if(p.size()<3)return p;
    sort(p.begin(),p.end(),pointLess);
    vector<Point> h;
    for(size_t i=0;i<p.size();++i){
        while(h.size()>=2&&crossP(h[h.size()-2],h.back(),p[i])<=0)h.pop_back();
        h.push_back(p[i]);
    }
    size_t lower=h.size();
    for(int i=(int)p.size()-2;i>=0;--i){
        while(h.size()>lower&&crossP(h[h.size()-2],h.back(),p[i])<=0)h.pop_back();
        h.push_back(p[i]);
    }
    if(h.size()>1)h.pop_back();
    return h;
}

Shadow buildingShadow(const Building& b){
    float x1=b.x-b.w/2,x2=b.x+b.w/2,z1=b.z-b.d/2,z2=b.z+b.d/2;
    vector<Point> p;
    p.push_back(projectShadow(x1,b.h,z1));p.push_back(projectShadow(x2,b.h,z1));
    p.push_back(projectShadow(x1,b.h,z2));p.push_back(projectShadow(x2,b.h,z2));
    p.push_back({x1,z1});p.push_back({x2,z1});p.push_back({x1,z2});p.push_back({x2,z2});
    Shadow s;s.p=hull(p);return s;
}

Shadow treeShadow(const Tree& tr){
    vector<Point> p;
    float canopyY=tr.height;
    for(int i=0;i<48;i++){
        float a=2*PI*i/48.0f;
        float x=tr.x+tr.radius*cos(a),z=tr.z+tr.radius*sin(a);
        p.push_back(projectShadow(x,canopyY,z));
    }
    Shadow s;s.p=hull(p);return s;
}

bool inside(float x,float z,const vector<Point>& p){
    bool in=false;int n=(int)p.size();if(n<3)return false;
    for(int i=0,j=n-1;i<n;j=i++){
        float xi=p[i].x,zi=p[i].z,xj=p[j].x,zj=p[j].z;
        if(((zi>z)!=(zj>z))&&(x<(xj-xi)*(z-zi)/((zj-zi)+0.00001f)+xi))in=!in;
    }
    return in;
}
bool isShaded(float x,float z){
    for(size_t i=0;i<shadows.size();++i)if(inside(x,z,shadows[i].p))return true;
    return false;
}

float computeCoverage(bool buildList=true){
    if(buildList){
        shadows.clear();
        for(size_t i=0;i<buildings.size();++i)shadows.push_back(buildingShadow(buildings[i]));
        for(size_t i=0;i<trees.size();++i)
            if(!trees[i].proposed||showProposed)shadows.push_back(treeShadow(trees[i]));
    }
    int shade=0,total=0;
    for(int side=-1;side<=1;side+=2){
        for(float x=3.25f;x<=4.95f;x+=0.2f){
            for(float z=-18;z<=18;z+=0.5f){
                ++total;if(isShaded(side*x,z))++shade;
            }
        }
    }
    shadedSamples=shade;totalSamples=total;
    shadePct=total?100.0f*shade/total:0;
    // Each sample represents approximately 0.1 square metres.
    shadedArea=shade*0.1f;exposedArea=(total-shade)*0.1f;
    return shadePct;
}

void drawBox(float x1,float y1,float z1,float x2,float y2,float z2,float r,float g,float b){
    glColor3f(r,g,b);
    glBegin(GL_QUADS);
    glVertex3f(x1,y1,z1);glVertex3f(x2,y1,z1);glVertex3f(x2,y1,z2);glVertex3f(x1,y1,z2);
    glVertex3f(x1,y2,z1);glVertex3f(x1,y2,z2);glVertex3f(x2,y2,z2);glVertex3f(x2,y2,z1);
    glVertex3f(x1,y1,z1);glVertex3f(x1,y2,z1);glVertex3f(x2,y2,z1);glVertex3f(x2,y1,z1);
    glVertex3f(x1,y1,z2);glVertex3f(x2,y1,z2);glVertex3f(x2,y2,z2);glVertex3f(x1,y2,z2);
    glVertex3f(x1,y1,z1);glVertex3f(x1,y1,z2);glVertex3f(x1,y2,z2);glVertex3f(x1,y2,z1);
    glVertex3f(x2,y1,z1);glVertex3f(x2,y2,z1);glVertex3f(x2,y2,z2);glVertex3f(x2,y1,z2);
    glEnd();
}

void drawGround(){
    drawBox(-26,-.12f,-26,26,-.02f,26,.27f,.43f,.28f);
    if(showGrid){
        glColor4f(.75f,.85f,.75f,.18f);glBegin(GL_LINES);
        for(int i=-25;i<=25;i+=5){
            glVertex3f((float)i,.005f,-25);glVertex3f((float)i,.005f,25);
            glVertex3f(-25,.005f,(float)i);glVertex3f(25,.005f,(float)i);
        }glEnd();
    }
}
void drawRoad(){
    drawBox(-3,.01f,-25,3,.07f,25,.16f,.17f,.19f);
    glColor3f(.95f,.91f,.68f);
    for(float z=-23;z<=23;z+=5){
        glBegin(GL_QUADS);glVertex3f(-.09f,.085f,z);glVertex3f(.09f,.085f,z);
        glVertex3f(.09f,.085f,z+2);glVertex3f(-.09f,.085f,z+2);glEnd();
    }
}
void drawFootpaths(){
    for(int side=-1;side<=1;side+=2){
        drawBox(side<0?-5.2f:3.0f,.02f,-25,side<0?-3.0f:5.2f,.09f,25,.66f,.64f,.58f);
    }
}
void drawShadows(){
    if(!showShadows)return;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    glColor4f(.045f,.055f,.09f,.48f);
    for(size_t i=0;i<shadows.size();++i){
        if(shadows[i].p.size()<3)continue;
        glBegin(GL_POLYGON);for(size_t j=0;j<shadows[i].p.size();++j)
            glVertex3f(shadows[i].p[j].x,.105f,shadows[i].p[j].z);
        glEnd();
    }
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}
void drawHeatmap(){
    if(!showHeatmap)return;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int side=-1;side<=1;side+=2){
        for(float x=3.25f;x<=4.95f;x+=.2f){
            for(float z=-18;z<=18;z+=.5f){
                bool sh=isShaded(side*x,z);
                if(sh)glColor4f(.05f,.85f,.28f,.30f);else glColor4f(.95f,.12f,.08f,.34f);
                glBegin(GL_QUADS);
                glVertex3f(side*x-.095f,.13f,z-.24f);glVertex3f(side*x+.095f,.13f,z-.24f);
                glVertex3f(side*x+.095f,.13f,z+.24f);glVertex3f(side*x-.095f,.13f,z+.24f);
                glEnd();
            }
        }
    }
    glDisable(GL_BLEND);
}
void drawBuildings(){
    for(size_t i=0;i<buildings.size();++i){
        const Building& b=buildings[i];
        drawBox(b.x-b.w/2,0,b.z-b.d/2,b.x+b.w/2,b.h,b.z+b.d/2,b.r,b.g,b.b);
        // Repeating facade windows on the street-facing wall.
        float face=(b.x<0)?b.x+b.w/2+.012f:b.x-b.w/2-.012f;
        glColor3f(.20f,.43f,.58f);
        for(float y=2;y<b.h-1;y+=3)for(float z=b.z-b.d/2+1;z<=b.z+b.d/2-1;z+=2){
            glBegin(GL_QUADS);
            glVertex3f(face,y,z-.38f);glVertex3f(face,y,z+.38f);
            glVertex3f(face,y+1.15f,z+.38f);glVertex3f(face,y+1.15f,z-.38f);
            glEnd();
        }
    }
}
void drawTree(const Tree& t){
    glPushMatrix();glTranslatef(t.x,0,t.z);
    glColor3f(.34f,.17f,.07f);glPushMatrix();glTranslatef(0,t.height*.38f,0);
    glScalef(.34f,t.height*.76f,.34f);glutSolidCube(1);glPopMatrix();
    if(t.proposed)glColor3f(.20f,.72f,.30f);else glColor3f(.045f,.36f,.12f);
    glPushMatrix();glTranslatef(0,t.height,0);glutSolidSphere(t.radius,22,18);glPopMatrix();
    glPopMatrix();
}
void drawSun(){
    glPushMatrix();glTranslatef(sunX,sunY,sunZ);glColor3f(1,.78f,.12f);
    glutSolidSphere(.85,22,18);glPopMatrix();
    // Sun direction marker
    glDisable(GL_LIGHTING);glColor3f(1,.82f,.25f);
    glBegin(GL_LINES);glVertex3f(sunX,sunY,sunZ);glVertex3f(sunX*.65f,0,sunZ*.65f);glEnd();
}
void drawCursor(){
    glPushMatrix();glTranslatef(cursorX,.25f,cursorZ);glColor3f(.15f,1,.75f);
    glutWireSphere(.65,14,12);glPopMatrix();
}

void text(float x,float y,const string& s,void* font=GLUT_BITMAP_HELVETICA_12){
    glRasterPos2f(x,y);for(size_t i=0;i<s.size();++i)glutBitmapCharacter(font,s[i]);
}
void rect(float x,float y,float w,float h,float r,float g,float b){
    glColor3f(r,g,b);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
void line(float x1,float y1,float x2,float y2,float r,float g,float b){
    glColor3f(r,g,b);glBegin(GL_LINES);glVertex2f(x1,y1);glVertex2f(x2,y2);glEnd();
}
void panelText(float x,float y,const string& s,float r=.88f,float g=.92f,float b=.95f){
    glColor3f(r,g,b);text(x,y,s,GLUT_BITMAP_HELVETICA_12);
}
void button(float x,float y,float w,float h,const string& label,bool active=false){
    rect(x,y,w,h,active?.12f:.16f,active?.39f:.28f,active?.34f:.34f);
    panelText(x+10,y+h/2-4,label,active?.95f:.85f,active?.98f:.90f,active?1.0f:.95f);
}
void drawUI(){
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,winW,0,winH);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_DEPTH_TEST);
    rect(0,0,(float)SIDEBAR,(float)winH,.055f,.085f,.12f);
    rect(0,winH-76,(float)SIDEBAR,76,.075f,.13f,.18f);
    panelText(20,winH-34,"SHADOW LAB",.98f,.99f,1);
    panelText(20,winH-54,"URBAN SHADE PLANNING",.48f,.78f,.83f);
    rect(18,winH-101,274,1,.23f,.34f,.40f);
    panelText(20,winH-125,"LIVE ANALYSIS",.48f,.82f,.84f);
    // Metric cards
    rect(18,winH-195,132,55,.12f,.18f,.23f);
    rect(158,winH-195,134,55,.12f,.18f,.23f);
    stringstream a;a<<fixed<<setprecision(1)<<shadePct<<"%";
    panelText(28,winH-163,a.str(),.36f,.92f,.64f);
    panelText(28,winH-182,"SHADE COVERAGE",.72f,.79f,.83f);
    stringstream b;b<<fixed<<setprecision(1)<<exposedArea<<" m2";
    panelText(168,winH-163,b.str(),1,.62f,.43f);
    panelText(168,winH-182,"EXPOSED AREA",.72f,.79f,.83f);
    panelText(20,winH-222,"TIME OF DAY",.48f,.82f,.84f);
    stringstream tm;tm<<setfill('0')<<setw(2)<<(int)simHour<<":"<<setw(2)<<(int)((simHour-(int)simHour)*60);
    panelText(236,winH-222,tm.str(),1,.83f,.43f);
    rect(22,winH-249,264,6,.24f,.31f,.36f);
    float sx=22+(simHour-6)/12.0f*264;
    rect(sx-4,winH-254,8,16,.98f,.75f,.25f);
    panelText(20,winH-270,"6 AM",.62f,.70f,.74f);panelText(246,winH-270,"6 PM",.62f,.70f,.74f);
    button(20,winH-308,82,27,"SUNRISE",simHour<8);
    button(111,winH-308,82,27,"NOON",simHour>=11&&simHour<=13);
    button(202,winH-308,88,27,"SUNSET",simHour>16);
    panelText(20,winH-339,"SCENE LAYERS",.48f,.82f,.84f);
    button(20,winH-375,128,27,showHeatmap?"HEATMAP  ON":"HEATMAP OFF",showHeatmap);
    button(160,winH-375,130,27,showShadows?"SHADOWS ON":"SHADOWS OFF",showShadows);
    button(20,winH-409,128,27,showGrid?"GRID ON":"GRID OFF",showGrid);
    button(160,winH-409,130,27,topView?"3D VIEW":"TOP VIEW",topView);
    panelText(20,winH-442,"TREE PLANNING",.48f,.82f,.84f);
    stringstream tr;tr<<"Trees: "<<trees.size()<<"  (green = proposed)";
    panelText(20,winH-462,tr.str());
    button(20,winH-499,128,28,"PLANT TREE");
    button(160,winH-499,130,28,"REMOVE LAST");
    panelText(20,winH-526,"Cursor: W A S D  |  Enter = plant",.68f,.76f,.80f);
    panelText(20,winH-551,"[ ] radius   , . height",.68f,.76f,.80f);
    stringstream rr;rr<<"Canopy radius: "<<fixed<<setprecision(1)<<selectedTreeRadius<<" m";
    panelText(20,winH-568,rr.str(),.80f,.88f,.90f);
    stringstream hh;hh<<"Canopy height: "<<fixed<<setprecision(1)<<selectedTreeHeight<<" m";
    panelText(20,winH-585,hh.str(),.80f,.88f,.90f);
    panelText(20,winH-615,"OPTIMIZATION",.48f,.82f,.84f);
    button(20,winH-652,270,29,"FIND BEST TREE LOCATIONS");
    stringstream gain;gain<<"Best tested gain: +"<<fixed<<setprecision(1)<<bestGain<<"%";
    panelText(20,winH-675,gain.str(),.42f,.92f,.65f);
    button(20,winH-712,128,27,"APPLY BEST");
    button(160,winH-712,130,27,"SAVE BASELINE");
    panelText(20,winH-741,"BASELINE / COMPARISON",.48f,.82f,.84f);
    stringstream base;
    if(baselinePct>=0)base<<"Baseline: "<<fixed<<setprecision(1)<<baselinePct<<"%  |  Change: "<<(shadePct-baselinePct)<<"%";
    else base<<"Baseline not saved yet";
    panelText(20,winH-760,base.str(),.83f,.88f,.91f);
    // Status strip
    rect(SIDEBAR,0,(float)(winW-SIDEBAR),34,.055f,.085f,.12f);
    panelText(SIDEBAR+16,13,statusMessage,.78f,.87f,.89f);
    // Scene label
    rect(SIDEBAR+14,winH-42,230,27,.055f,.085f,.12f);
    panelText(SIDEBAR+26,winH-25,"3D STREET VIEW  /  LIVE MODEL",.86f,.93f,.95f);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);
}

void renderScene(){
    int viewW=winW-SIDEBAR;
    glViewport(SIDEBAR,34,viewW,winH-34);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluPerspective(48.0,(double)viewW/(double)(winH-34),.5,220.0);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    if(topView)gluLookAt(0,58,.01,0,0,0,0,0,-1);
    else{
        float a=cameraAngle*PI/180.0f;
        gluLookAt(cameraDistance*sin(a),cameraHeight,cameraDistance*cos(a),0,0,0,0,1,0);
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    computeCoverage(true);
    drawGround();drawRoad();drawFootpaths();
    drawShadows();drawHeatmap();drawBuildings();
    for(size_t i=0;i<trees.size();++i)if(!trees[i].proposed||showProposed)drawTree(trees[i]);
    drawSun();drawCursor();
}

float evaluateWithCandidate(float x,float z){
    trees.push_back({x,z,selectedTreeHeight,selectedTreeRadius,true});
    computeCoverage(true);float result=shadePct;
    trees.pop_back();computeCoverage(true);
    return result;
}
void findRecommendations(){
    computeCoverage(true);
    float current=shadePct,best=current;
    Point candidate={cursorX,cursorZ};
    // Search legal planting strips, avoiding the existing tree trunks.
    for(int side=-1;side<=1;side+=2){
        for(float x=3.8f;x<=4.7f;x+=.45f){
            for(float z=-17;z<=17;z+=2.0f){
                float px=side*x;bool occupied=false;
                for(size_t k=0;k<trees.size();++k)
                    if(fabs(trees[k].x-px)<1.0f&&fabs(trees[k].z-z)<1.2f)occupied=true;
                if(!occupied){
                    float score=evaluateWithCandidate(px,z);
                    if(score>best){best=score;candidate={px,z};}
                }
            }
        }
    }
    bestSpot=candidate;bestGain=best-current;cursorX=bestSpot.x;cursorZ=bestSpot.z;
    statusMessage="Best candidate found. Press APPLY BEST to add the recommended tree.";
    glutPostRedisplay();
}
void addTree(float x,float z){
    if(fabs(x)<3.25f||fabs(x)>5.0f||z<-18||z>18){
        statusMessage="Choose a planting position on either pedestrian footpath.";return;
    }
    for(size_t i=0;i<trees.size();++i)
        if(fabs(trees[i].x-x)<.9f&&fabs(trees[i].z-z)<1.0f){
            statusMessage="Too close to an existing tree. Move the cursor.";return;
        }
    trees.push_back({x,z,selectedTreeHeight,selectedTreeRadius,true});
    statusMessage="Proposed tree added. Shade analysis updated.";
}
void removeLast(){
    for(int i=(int)trees.size()-1;i>=0;--i)if(trees[i].proposed){
        trees.erase(trees.begin()+i);statusMessage="Last proposed tree removed.";return;
    }
    statusMessage="There are no proposed trees to remove.";
}

void display(){
    glClearColor(.48f,.68f,.82f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    renderScene();glViewport(0,0,winW,winH);drawUI();glutSwapBuffers();
}
void reshape(int w,int h){winW=w;winH=h>1?h:1;glViewport(0,0,w,h);glutPostRedisplay();}

void keyboard(unsigned char key,int,int){
    switch(key){
        case 27:exit(0);
        case 'w':case 'W':cursorZ-=1;break;
        case 's':case 'S':cursorZ+=1;break;
        case 'a':case 'A':cursorX-=.35f;break;
        case 'd':case 'D':cursorX+=.35f;break;
        case 13:addTree(cursorX,cursorZ);break;
        case 8:case 127:removeLast();break;
        case 'h':case 'H':showHeatmap=!showHeatmap;break;
        case 'g':case 'G':showGrid=!showGrid;break;
        case 't':case 'T':topView=!topView;break;
        case 'b':case 'B':baselinePct=shadePct;statusMessage="Baseline saved.";break;
        case '[':selectedTreeRadius=max(1.0f,selectedTreeRadius-.2f);break;
        case ']':selectedTreeRadius=min(3.5f,selectedTreeRadius+.2f);break;
        case ',':selectedTreeHeight=max(2.0f,selectedTreeHeight-.2f);break;
        case '.':selectedTreeHeight=min(7.0f,selectedTreeHeight+.2f);break;
        case '+':case '=':cameraDistance=max(22.0f,cameraDistance-2);break;
        case '-':case '_':cameraDistance=min(85.0f,cameraDistance+2);break;
        case 'r':case 'R':cameraAngle=42;cameraHeight=24;cameraDistance=43;topView=false;break;
        case ' ':autoRotate=!autoRotate;break;
    }
    glutPostRedisplay();
}
void specialKeys(int key,int,int){
    if(key==GLUT_KEY_LEFT)cameraAngle-=5;
    if(key==GLUT_KEY_RIGHT)cameraAngle+=5;
    if(key==GLUT_KEY_UP)cameraHeight=min(55.0f,cameraHeight+1);
    if(key==GLUT_KEY_DOWN)cameraHeight=max(7.0f,cameraHeight-1);
    glutPostRedisplay();
}
void setHourFromMouse(int x){
    float local=(float)(x-SIDEBAR-22);
    float t=local/(float)((winW-SIDEBAR)-44);
    if(t<0)t=0;if(t>1)t=1;
    simHour=6+12*t;updateSun();computeCoverage(true);
}
void mouse(int buttonID,int state,int x,int y){
    int yy=winH-y;
    if(buttonID==GLUT_LEFT_BUTTON&&state==GLUT_DOWN){
        if(x>=SIDEBAR+22&&yy>=winH-254&&yy<=winH-230){draggingTime=true;setHourFromMouse(x);glutPostRedisplay();return;}
        if(x<SIDEBAR){
            if(yy>=winH-308&&yy<=winH-281){
                if(x<105)simHour=7;else if(x<198)simHour=12;else simHour=17;updateSun();
            }else if(yy>=winH-375&&yy<=winH-348){
                if(x<150)showHeatmap=!showHeatmap;else showShadows=!showShadows;
            }else if(yy>=winH-409&&yy<=winH-382){
                if(x<150)showGrid=!showGrid;else topView=!topView;
            }else if(yy>=winH-499&&yy<=winH-471){
                if(x<155)addTree(cursorX,cursorZ);else removeLast();
            }else if(yy>=winH-652&&yy<=winH-623){
                findRecommendations();
            }else if(yy>=winH-712&&yy<=winH-685){
                if(x<155){addTree(bestSpot.x,bestSpot.z);}else{baselinePct=shadePct;statusMessage="Current design saved as baseline.";}
            }
            glutPostRedisplay();return;
        }
    }
    if(buttonID==GLUT_LEFT_BUTTON&&state==GLUT_UP)draggingTime=false;
    if(buttonID==3){cameraDistance=max(22.0f,cameraDistance-2);glutPostRedisplay();}
    if(buttonID==4){cameraDistance=min(85.0f,cameraDistance+2);glutPostRedisplay();}
}
void motion(int x,int){
    if(draggingTime){setHourFromMouse(x);glutPostRedisplay();}
}
void idle(){
    if(autoRotate){cameraAngle+=.12f;if(cameraAngle>360)cameraAngle-=360;}
    glutPostRedisplay();
}

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(winW,winH);
    glutInitWindowPosition(60,35);
    glutCreateWindow("SHADOW LAB | Urban Shade Planning Studio");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    initCity();updateSun();computeCoverage(true);
    glutDisplayFunc(display);glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);glutMotionFunc(motion);glutPassiveMotionFunc(motion);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
