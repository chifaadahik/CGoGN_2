/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#include <QApplication>
#include <QMatrix4x4>

#include <qoglviewer.h>
#include <QKeyEvent>

#include <core/cmap/cmap3.h>
#include <io/map_import.h>
#include <geometry/algos/bounding_box.h>
#include <rendering/shaders/vbo.h>
#include <rendering/map_render.h>
#include <rendering/drawer.h>
#include <rendering/volume_render.h>
#include <rendering/topo_render.h>
#include <geometry/algos/picking.h>



#define DEFAULT_MESH_PATH CGOGN_STR(CGOGN_TEST_MESHES_PATH)

using namespace cgogn::numerics;
using Map3 = cgogn::CMap3<cgogn::DefaultMapTraits>;
using Vec3 = Eigen::Vector3d;
//using Vec3 = cgogn::geometry::Vec_T<std::array<double,3>>;

template<typename T>
using VertexAttributeHandler = Map3::VertexAttributeHandler<T>;


class Viewer : public QOGLViewer
{
public:
	Viewer();
	Viewer(const Viewer&) = delete;
	Viewer& operator=(const Viewer&) = delete;

	virtual void draw();
	virtual void init();
	virtual void keyPressEvent(QKeyEvent*);
	virtual void mousePressEvent(QMouseEvent*);

	void import(const std::string& volumeMesh);
	virtual ~Viewer();
	virtual void closeEvent(QCloseEvent *e);

private:
	void rayClick(QMouseEvent* event, qoglviewer::Vec& P, qoglviewer::Vec& Q);

	Map3 map_;
	VertexAttributeHandler<Vec3> vertex_position_;

	cgogn::geometry::BoundingBox<Vec3> bb_;

	cgogn::rendering::VBO* vbo_pos_;

	cgogn::rendering::TopoRender* topo_render_;
	cgogn::rendering::VolumeRender* volume_render_;

	cgogn::rendering::Drawer* drawer_;

	bool vol_rendering_;
	bool edge_rendering_;
	bool topo_rendering_;

	float expl_;

};


//
// IMPLEMENTATION
//
void Viewer::rayClick(QMouseEvent* event, qoglviewer::Vec& P, qoglviewer::Vec& Q)
{
	P = camera()->unprojectedCoordinatesOf(qoglviewer::Vec(event->x(), event->y(), 0.0));
	Q = camera()->unprojectedCoordinatesOf(qoglviewer::Vec(event->x(), event->y(), 1.0));
}


void Viewer::import(const std::string& volumeMesh)
{
	cgogn::io::import_volume<Vec3>(map_, volumeMesh);

	vertex_position_ = map_.get_attribute<Vec3, Map3::Vertex::ORBIT>("position");

	cgogn::geometry::compute_bounding_box(vertex_position_, bb_);

	setSceneRadius(bb_.diag_size()/2.0);
	Vec3 center = bb_.center();
	setSceneCenter(qoglviewer::Vec(center[0], center[1], center[2]));
	showEntireScene();
}

Viewer::~Viewer()
{}

void Viewer::closeEvent(QCloseEvent*)
{
	delete vbo_pos_;
	delete topo_render_;
	delete volume_render_;
	delete drawer_;
}

Viewer::Viewer() :
	map_(),
	vertex_position_(),
	bb_(),
	vbo_pos_(nullptr),
	topo_render_(nullptr),
	volume_render_(nullptr),
	drawer_(nullptr),
	vol_rendering_(true),
	edge_rendering_(true),
	topo_rendering_(true),
	expl_(0.8f)
{}

void Viewer::keyPressEvent(QKeyEvent *ev)
{
	switch (ev->key()) {
		case Qt::Key_V:
			vol_rendering_ = !vol_rendering_;
			break;
		case Qt::Key_E:
			edge_rendering_ = !edge_rendering_;
			break;

		case Qt::Key_T:
			topo_rendering_ = !topo_rendering_;
			break;
		case Qt::Key_Plus:
			expl_ += 0.05f;
			volume_render_->set_explode_volume(expl_);
			topo_render_->set_explode_volume(expl_);
			topo_render_->update_map3<Vec3>(map_,vertex_position_);
			break;
		case Qt::Key_Minus:
			expl_ -= 0.05f;
			volume_render_->set_explode_volume(expl_);
			topo_render_->set_explode_volume(expl_);
			topo_render_->update_map3<Vec3>(map_,vertex_position_);
			break;
		default:
			break;
	}
	// enable QGLViewer keys
	QOGLViewer::keyPressEvent(ev);
	//update drawing
	update();
}

void Viewer::mousePressEvent(QMouseEvent* event)
{
	if (event->modifiers() & Qt::ShiftModifier)
	{
		qoglviewer::Vec P;
		qoglviewer::Vec Q;
		rayClick(event, P, Q);

		Vec3 A(P[0], P[1], P[2]);
		Vec3 B(Q[0], Q[1], Q[2]);

		drawer_->new_list();

		std::vector<Map3::Volume> selected;
		cgogn::geometry::picking_volumes<Vec3>(map_, vertex_position_, A, B, selected);
		std::cout << "Selected volumes: " << selected.size() << std::endl;
		if (!selected.empty())
		{
			drawer_->line_width(2.0);
			drawer_->begin(GL_LINES);
			// closest vol in red
			drawer_->color3f(1.0, 0.0, 0.0);
			cgogn::rendering::add_volume_to_drawer<Vec3>(map_, selected[0], vertex_position_, drawer_);
			// others in yellow
			drawer_->color3f(1.0, 1.0, 0.0);
			for (uint32 i = 1u; i<selected.size(); ++i)
				cgogn::rendering::add_volume_to_drawer<Vec3>(map_, selected[i], vertex_position_, drawer_);
			drawer_->end();
		}
		drawer_->line_width(4.0);
		drawer_->begin(GL_LINES);
		drawer_->color3f(1.0, 0.0, 1.0);
		drawer_->vertex3fv(A);
		drawer_->vertex3fv(B);
		drawer_->end();

		drawer_->end_list();
	}


	QOGLViewer::mousePressEvent(event);
}


void Viewer::draw()
{
	QMatrix4x4 proj;
	QMatrix4x4 view;
	camera()->getProjectionMatrix(proj);
	camera()->getModelViewMatrix(view);

	if (vol_rendering_)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		volume_render_->draw_faces(proj,view);

		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (edge_rendering_)
		volume_render_->draw_edges(proj,view);


	if (topo_rendering_)
		topo_render_->draw(proj,view);

	drawer_->call_list(proj, view);

}

void Viewer::init()
{
	glClearColor(0.1f,0.1f,0.3f,0.0f);

	vbo_pos_ = new cgogn::rendering::VBO(3);
	cgogn::rendering::update_vbo(vertex_position_, *vbo_pos_);

	topo_render_ = new cgogn::rendering::TopoRender(this);
	topo_render_->set_explode_volume(expl_);
	topo_render_->update_map3<Vec3>(map_,vertex_position_);

	volume_render_ = new cgogn::rendering::VolumeRender(this);
	volume_render_->set_explode_volume(expl_);
	volume_render_->update_face<Vec3>(map_,vertex_position_);
	volume_render_->update_edge<Vec3>(map_,vertex_position_);

	drawer_ = new cgogn::rendering::Drawer(this);
}

int main(int argc, char** argv)
{
	std::string volumeMesh;
	if (argc < 2)
	{
		std::cout << "USAGE: " << argv[0] << " [filename]" << std::endl;
		volumeMesh = std::string(DEFAULT_MESH_PATH) + std::string("nine_hexas.vtu");
		std::cout << "Using default mesh : " << volumeMesh << std::endl;
	}
	else
		volumeMesh = std::string(argv[1]);

	QApplication application(argc, argv);
	qoglviewer::init_ogl_context();

	// Instantiate the viewer.
	Viewer viewer;
	viewer.setWindowTitle("simpleViewer");
	viewer.import(volumeMesh);
	viewer.show();

	// Run main loop.
	return application.exec();
}