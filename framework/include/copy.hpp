#ifndef COPY_HPP
#define COPY_HPP

/*
 *
 * Copyright (c) 2013, Institute for Microelectronics, TU Wien.
 *
 * This file is part of ViennaMOS     http://viennamos.sourceforge.net/
 *
 * Contact: Josef Weinbub             weinbub@iue.tuwien.ac.at
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkDataArray.h>
#include <vtkAssignAttribute.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkTetra.h>

#include "forwards.hpp"

//#include "common.hpp"
//#include "device.hpp"
#include "multiview.h"
//#include "quantity.h"

//#include "boost/lexical_cast.hpp"

// remove me ?!
//#include "vtkDoubleArray.h"
//#include "vtkUnsignedCharArray.h"
//#include "vtkFloatArray.h"
#include <QDebug>

// ViennaMini includes
//
#include "viennamini/device.hpp"

namespace viennamos {

  class copy_exception : public std::runtime_error {
  public:
    copy_exception(std::string const & str) : std::runtime_error(str) {}
  };

  template<typename MeshT, typename SegmentationT>
  void copy(viennagrid::segmented_mesh<MeshT, SegmentationT>& segmesh, MultiView* multiview)
  {
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }


    typedef MeshT                                                             MeshType;
    typedef SegmentationT                                                     SegmentationType;
    typedef typename SegmentationType::segment_handle_type                    SegmentType;
    typedef typename viennagrid::result_of::vertex<MeshType>::type            VertexType;
    typedef typename viennagrid::result_of::vertex_range<SegmentType>::type   VertexRange;
    typedef typename viennagrid::result_of::iterator<VertexRange>::type       VertexIterator;
    typedef typename viennagrid::result_of::cell_range<SegmentType>::type     CellRange;
    typedef typename viennagrid::result_of::iterator<CellRange>::type         CellIterator;
    typedef typename viennagrid::result_of::cell<MeshType>::type              CellType;
    typedef typename viennagrid::result_of::cell_tag<MeshType>::type          CellTagType;
    typedef typename viennagrid::result_of::vertex_range<CellType>::type  VertexOnCellRange;
    typedef typename viennagrid::result_of::iterator<VertexOnCellRange>::type                                   VertexOnCellIterator;

    MeshType         & mesh         = segmesh.mesh;
    SegmentationType & segmentation = segmesh.segmentation;

    std::string cell_type = CellTagType::name();
    static int DIMG       = typename viennagrid::result_of::point<MeshT>::type().size();
    static int CELL_SIZE  = viennagrid::boundary_elements<CellTagType, viennagrid::vertex_tag>::num;

    int VTK_CELL_TYPE;
    if(cell_type == "2-simplex")
      VTK_CELL_TYPE = VTK_TRIANGLE;
    else
    if(cell_type == "3-simplex")
      VTK_CELL_TYPE = VTK_TETRA;
    else
    {
      throw copy_exception("ViennaGrid device not supported!");
      return;
    }

    multiview->resetGrid();
    MultiView::MultiGrid multigrid = multiview->getGrid();

    std::size_t segment_index = 0;
    for(typename SegmentationType::iterator sit = segmentation.begin();
        sit != segmentation.end(); sit++)
    {
      // for each segment, setup an unstructured vtk grid
      // and add it as a new block to the central multi-block datastructure
      //
      vtkUnstructuredGrid* usg = vtkUnstructuredGrid::New();

      // transfer the segment's geometry information
      //
      vtkPoints* points = vtkPoints::New();
      std::map<std::size_t,std::size_t>   indexMap;
      double temp_point[3];
      temp_point[0] = 0.0;
      temp_point[1] = 0.0;
      temp_point[2] = 0.0;
      int i = 0;
      VertexRange vertices = viennagrid::elements<VertexType>(*sit);
      for (VertexIterator vit = vertices.begin(); vit != vertices.end(); ++vit)
      {
        for(int dim = 0; dim < DIMG; dim++)
          temp_point[dim] = viennagrid::point(*vit)[dim];
        points->InsertNextPoint(temp_point);
        indexMap[vit->id().get()] = i++;
      }
      usg->SetPoints(points);
      points->Delete();

      // transfer the segment's topology information
      //
      CellRange cells = viennagrid::elements<CellType>(*sit);
      for (CellIterator cit = cells.begin(); cit != cells.end(); ++cit)
      {
        vtkIdType temp_cell[CELL_SIZE];
        for(int ci = 0; ci < CELL_SIZE; ci++)
        {
            temp_cell[ci] = indexMap.at( viennagrid::vertices(*cit)[ci].id().get() );
        }
        usg->InsertNextCell (VTK_CELL_TYPE , CELL_SIZE, temp_cell );
      }
      // now, as the grid representing the segment has been set up,
      // add it as a new block to the central multi-block datastructure
      //
      multigrid->SetBlock(segment_index++, usg);

      // we don't need the usg anymore,
      // delete everything, we'll start from scratch with the next segment ..
      //
      usg->Delete();
    }
    multiview->multigridModified();
  }

  inline void copy(viennamini::device_handle& vmini_device, MultiView* multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }

    if(vmini_device->is_line1d())
      viennamos::copy(vmini_device->get_segmesh_line_1d(),        multiview);
    if(vmini_device->is_triangular2d())
      viennamos::copy(vmini_device->get_segmesh_triangular_2d(),  multiview);
    else if(vmini_device->is_tetrahedral3d())
      viennamos::copy(vmini_device->get_segmesh_tetrahedral_3d(), multiview);
  }

  //
  // Vertex Quantity Transfer
  //
  template<typename MeshT, typename SegmentationT, typename VGridConfigT, typename ValueT>
  void copy(viennagrid::segmented_mesh<MeshT, SegmentationT> & segmesh,
            viennafvm::quantity<  viennagrid::element<viennagrid::simplex_tag<0>, VGridConfigT>, ValueT>         & quantity,
            MultiView                                        * multiview)
  {
    typedef typename SegmentationT::segment_handle_type                                                     SegmentType;
    typedef typename viennagrid::result_of::element<SegmentType, viennagrid::vertex_tag>::type              VertexType;
    typedef typename viennagrid::result_of::element_range<SegmentType, viennagrid::vertex_tag>::type  VertexRange;
    typedef typename viennagrid::result_of::iterator<VertexRange>::type                                     VertexIterator;

    SegmentationT & segmentation = segmesh.segmentation;

    MultiView::MultiGrid multigrid = multiview->getGrid();

    std::string quantity_name = quantity.get_name();

    std::size_t si = 0;
    for(typename SegmentationT::iterator sit = segmentation.begin();
        sit != segmentation.end(); sit++)
    {
      VertexRange vertices = viennagrid::elements<VertexType>(*sit);
      vtkSmartPointer<vtkDoubleArray> render_data = vtkSmartPointer<vtkDoubleArray>::New();
      render_data->SetName(quantity_name.c_str());
      render_data->SetNumberOfValues(vertices.size());
      std::size_t i = 0;
      for(VertexIterator vit = vertices.begin(); vit != vertices.end(); vit++)
      {
        render_data->SetValue(i++,quantity(*vit));
      }
      vtkPointSet* generic_segment = vtkPointSet::SafeDownCast(multigrid->GetBlock(si));

      if(generic_segment->GetPointData()->HasArray(quantity_name.c_str()))
          generic_segment->GetPointData()->RemoveArray(quantity_name.c_str());
      generic_segment->GetPointData()->AddArray(render_data);

      si++;
    }
  }

  //
  // Cell Quantity Transfer
  //
  template<typename MeshT, typename SegmentationT, typename VGridConfigT, typename ValueT>
  void copy(viennagrid::segmented_mesh<MeshT, SegmentationT> & segmesh,
            viennafvm::quantity<  viennagrid::element<typename viennagrid::result_of::cell_tag<MeshT>::type, VGridConfigT>, ValueT>         & quantity,
            MultiView                                        * multiview)
  {
    typedef typename SegmentationT::segment_handle_type                                                     SegmentType;
    typedef typename viennagrid::result_of::cell_tag<MeshT>::type                                           CellTag;
    typedef typename viennagrid::result_of::element<MeshT, CellTag>::type                                   CellType;
    typedef typename viennagrid::result_of::element_range<SegmentType, CellTag>::type                       CellRange;
    typedef typename viennagrid::result_of::iterator<CellRange>::type                                       CellIterator;

    SegmentationT & segmentation = segmesh.segmentation;

    MultiView::MultiGrid multigrid = multiview->getGrid();

    std::string quantity_name = quantity.get_name();

    std::size_t si = 0;
    for(typename SegmentationT::iterator sit = segmentation.begin();
        sit != segmentation.end(); sit++)
    {
      CellRange cells = viennagrid::elements<CellType>(*sit);
      vtkSmartPointer<vtkDoubleArray> render_data = vtkSmartPointer<vtkDoubleArray>::New();
      render_data->SetName(quantity_name.c_str());
      render_data->SetNumberOfValues(cells.size());
      std::size_t i = 0;
      for(CellIterator cit = cells.begin(); cit != cells.end(); cit++)
      {
        render_data->SetValue(i++, quantity(*cit));
      }
      vtkPointSet* generic_segment = vtkPointSet::SafeDownCast(multigrid->GetBlock(si));

      if(generic_segment->GetCellData()->HasArray(quantity_name.c_str()))
          generic_segment->GetCellData()->RemoveArray(quantity_name.c_str());
      generic_segment->GetCellData()->AddArray(render_data);

      si++;
    }
  }

  //
  // Convenience Vertex Quantity Transfer Overload for 1D Meshes
  //
  template<typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<viennagrid::simplex_tag<0>, viennagrid::config::line_1d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }

    if(vmini_device->is_line1d())
      viennamos::copy(vmini_device->get_segmesh_line_1d(),  quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }

  //
  // Convenience Vertex Quantity Transfer Overload for 2D Triangular Meshes
  //
  template<typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<viennagrid::simplex_tag<0>, viennagrid::config::triangular_2d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }

    if(vmini_device->is_triangular2d())
      viennamos::copy(vmini_device->get_segmesh_triangular_2d(),  quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }

  //
  // Convenience Vertex Quantity Transfer Overload for 3D Tetrahedral Meshes
  //
  template<typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<viennagrid::simplex_tag<0>, viennagrid::config::tetrahedral_3d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }
    if(vmini_device->is_tetrahedral3d())
      viennamos::copy(vmini_device->get_segmesh_tetrahedral_3d(), quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }

  //
  // Convenience Cell Quantity Transfer Overload for 1D Meshes
  //
  template<typename CellTagT, typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<CellTagT, viennagrid::config::line_1d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }

    if(vmini_device->is_line1d())
      viennamos::copy(vmini_device->get_segmesh_line_1d(),  quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }

  //
  // Convenience Cell Quantity Transfer Overload for 2D Triangular Meshes
  //
  template<typename CellTagT, typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<CellTagT, viennagrid::config::triangular_2d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }

    if(vmini_device->is_triangular2d())
      viennamos::copy(vmini_device->get_segmesh_triangular_2d(),  quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }

  //
  // Convenience Cell Quantity Transfer Overload for 3D Tetrahedral Meshes
  //
  template<typename CellTagT, typename ValueT>
  void copy(viennamini::device_handle                & vmini_device,
            viennafvm::quantity<  viennagrid::element<CellTagT, viennagrid::config::tetrahedral_3d>, ValueT> & quantity,
            MultiView                                * multiview)
  {
    if(!vmini_device)
    {
      throw copy_exception("Device is not available");
      return;
    }
    if(!multiview)
    {
      throw copy_exception("MultiView is not available");
      return;
    }
    if(vmini_device->is_tetrahedral3d())
      viennamos::copy(vmini_device->get_segmesh_tetrahedral_3d(), quantity, multiview);
    else throw copy_exception("Mesh does not fit to quantity");
  }


//template<typename PostfixT>
//inline std::string combine(std::string str, PostfixT const& postfix)
//{
//    return str+boost::lexical_cast<std::string>(postfix);
//}

//inline void copy(vtkSmartPointer<vtkStructuredGrid>& structured_grid, MultiView* multiview)
//{

//    multiview->resetGrid();
//    MultiView::Grid grid = multiview->getGrid();

//    grid->SetPoints(structured_grid->GetPoints());

//    for(long ci = 0; ci < structured_grid->GetNumberOfCells(); ci++)
//    {
//        grid->InsertNextCell(VTK_QUAD, structured_grid->GetCell(ci)->GetPointIds());
//    }
//}

//inline void copy(Quantity const& quantity, MultiView* multiview)
//{
//    MultiView::Grid grid = multiview->getGrid();

//    std::string  const& key           = quantity.key;
//    std::string  const& source        = quantity.source;
//    std::string  const& unit          = quantity.unit;
//    int          const& cell_lvl      = quantity.cell_lvl;
//    int          const& tensor_lvl    = quantity.tensor_lvl;
//    DataSequence const& data_sequence = quantity.data_sequence;

//    std::string merged_key = quantity.generate_key_string();

//    if( (cell_lvl == VERTEX) && (tensor_lvl == SCALAR) )
//    {
//        int id = 0;
//        for(DataSequence::const_iterator diter = data_sequence.begin(); diter != data_sequence.end(); diter++)
//        {
//            // set the name of the vtk array to be the merged key and the index indicating the
//            // position in the data sequence -> this way we can access the data at a later point
//            // as the renderer holds the whole result-sequence in the memory ..
//            //
//            const char* actual_key = viennamos::combine(merged_key, id++).c_str();

//            (*diter)->SetName(actual_key);

//            // now add the data array to the vtk grid - remove any previous data with the same key
//            //
//            if(grid->GetPointData()->HasArray(actual_key))
//                grid->GetPointData()->RemoveArray(actual_key);
//            grid->GetPointData()->AddArray(*diter);
//        }
//    }
//    else if( (cell_lvl == CELL) && (tensor_lvl == SCALAR) )
//    {
//        int id = 0;
//        for(DataSequence::const_iterator diter = data_sequence.begin(); diter != data_sequence.end(); diter++)
//        {
//            // set the name of the vtk array to be the merged key and the index indicating the
//            // position in the data sequence -> this way we can access the data at a later point
//            // as the renderer holds the whole result-sequence in the memory ..
//            //
//            const char* actual_key = viennamos::combine(merged_key, id++).c_str();

//            (*diter)->SetName(actual_key);

//            // now add the data array to the vtk grid - remove any previous data with the same key
//            //
//            if(grid->GetCellData()->HasArray(actual_key))
//                grid->GetCellData()->RemoveArray(actual_key);
//            grid->GetCellData()->AddArray(*diter);
//        }
//    }
//    else
//    {
//        std::cout << "copy not implemented for this kind of quantity:" <<
//                     key << " - " << source << " - " << unit << " - " << cell_lvl << " - " << tensor_lvl << std::endl;
//    }
//}

//inline void copy(QuantitySet& source, MultiView* multiview)
//{
//    for(QuantitySet::iterator iter = source.begin(); iter != source.end(); iter++)
//    {
//        viennamos::copy(*iter, multiview);
//    }
//}

//template<typename DeviceT>
//inline void copy(DeviceT& device, Quantity const& quantity, MultiView* multiview)
//{
//  typedef typename DeviceT::CellComplex                   DomainType;
//  typedef typename DeviceT::Segmentation                  SegmentationType;
//  typedef typename DeviceT::QuantityComplex               QuantityComplexType;
//  typedef typename SegmentationType::iterator             SegmentationIteratorType;
//  typedef typename SegmentationType::segment_handle_type  SegmentType;

//  typedef typename viennagrid::result_of::cell_tag<DomainType>::type                                  CellTag;
//  typedef typename viennagrid::result_of::element<DomainType, CellTag>::type                          CellType;
//  typedef typename viennagrid::result_of::element<SegmentType, viennagrid::vertex_tag>::type          VertexType;
//  typedef typename viennagrid::result_of::element_range<SegmentType, viennagrid::vertex_tag>::type    VertexRange;
//  typedef typename viennagrid::result_of::iterator<VertexRange>::type                                 VertexIterator;
//  typedef typename viennagrid::result_of::element_range<SegmentType, CellTag>::type                   CellRange;
//  typedef typename viennagrid::result_of::iterator<CellRange>::type                                   CellIterator;

//  MultiView::MultiGrid   multigrid = multiview->getGrid();
//  SegmentationType     & segments  = device.getSegmentation();


//  if((quantity.cell_level == VERTEX) && (quantity.tensor_level == SCALAR))
//  {
//    typedef typename viennadata::result_of::accessor<QuantityComplexType, Quantity, double, VertexType>::type TargetAccessorType;
//    TargetAccessorType accessor = viennadata::make_accessor(device.getQuantityComplex(), quantity);

//    std::size_t si = 0;
//    for(SegmentationIteratorType sit = segments.begin(); sit != segments.end(); sit++)
//    {
//      VertexRange vertices = viennagrid::elements<VertexType>(*sit);
//      vtkSmartPointer<vtkDoubleArray> render_data = vtkSmartPointer<vtkDoubleArray>::New();
//      render_data->SetName(quantity.name.c_str());
//      render_data->SetNumberOfValues(vertices.size());

//      std::size_t i = 0;
//      for(VertexIterator vit = vertices.begin(); vit != vertices.end(); vit++)
//      {
//        render_data->SetValue(i++, accessor(*vit));
//      }

//      vtkPointSet* generic_segment = vtkPointSet::SafeDownCast(multigrid->GetBlock(si));

//      if(generic_segment->GetPointData()->HasArray(quantity.name.c_str()))
//          generic_segment->GetPointData()->RemoveArray(quantity.name.c_str());
//      generic_segment->GetPointData()->AddArray(render_data);

//      si++;
//    }
//  }
//  else
//  if((quantity.cell_level == CELL) && (quantity.tensor_level == SCALAR))
//  {
//    typedef typename viennadata::result_of::accessor<QuantityComplexType, Quantity, double, CellType>::type TargetAccessorType;
//    TargetAccessorType accessor = viennadata::make_accessor(device.getQuantityComplex(), quantity);

//    std::size_t si = 0;
//    for(SegmentationIteratorType sit = segments.begin(); sit != segments.end(); sit++)
//    {
//      CellRange cells = viennagrid::elements<CellType>(*sit);
//      vtkSmartPointer<vtkDoubleArray> render_data = vtkSmartPointer<vtkDoubleArray>::New();
//      render_data->SetName(quantity.name.c_str());
//      render_data->SetNumberOfValues(cells.size());

//      std::size_t i = 0;
//      for(CellIterator cit = cells.begin(); cit != cells.end(); cit++)
//      {
//        render_data->SetValue(i++, accessor(*cit));
//      }

//      vtkPointSet* generic_segment = vtkPointSet::SafeDownCast(multigrid->GetBlock(si));

//      if(generic_segment->GetCellData()->HasArray(quantity.name.c_str()))
//          generic_segment->GetCellData()->RemoveArray(quantity.name.c_str());
//      generic_segment->GetCellData()->AddArray(render_data);

//      si++;
//    }
//  }
//}

//template<typename DeviceT>
//inline void copy(DeviceT& device, MultiView* multiview, tag::viennagrid_domain, int VTK_CELL_TYPE)
//{
//  typedef typename DeviceT::CellComplex                     DomainType;
//  typedef typename DeviceT::Segmentation                    SegmentationType;
//  typedef typename DeviceT::QuantityComplex                 QuantityComplexType;
//  typedef typename SegmentationType::iterator               SegmentationIteratorType;
//  typedef typename SegmentationType::segment_handle_type    SegmentType;

//  typedef typename viennagrid::result_of::cell_tag<DomainType>::type                                  CellTag;
//  typedef typename viennagrid::result_of::element<DomainType, CellTag>::type                          CellType;
//  typedef typename viennagrid::result_of::element<SegmentType, viennagrid::vertex_tag>::type          VertexType;
//  typedef typename viennagrid::result_of::element_range<SegmentType, viennagrid::vertex_tag>::type    VertexRange;
//  typedef typename viennagrid::result_of::iterator<VertexRange>::type                                 VertexIterator;
//  typedef typename viennagrid::result_of::point<DomainType>::type                                     PointType;
//  typedef typename viennagrid::result_of::default_point_accessor<DomainType>::type                    PointAccessorType;
//  typedef typename viennagrid::result_of::element_range<SegmentType, CellTag>::type                   CellRange;
//  typedef typename viennagrid::result_of::iterator<CellRange>::type                                   CellIterator;
//  typedef typename viennagrid::result_of::element_range<CellType, VertexType>::type                   VertexOnCellRange;
//  typedef typename viennagrid::result_of::iterator<VertexOnCellRange>::type                           VertexOnCellIterator;

//  static const int DIMG = PointType::dim;

//  // TODO retrieve this info from viennagrid
//  //
//  int cell_size;
//  if(VTK_CELL_TYPE == VTK_TRIANGLE)
//      cell_size = 3;
//  else
//  if(VTK_CELL_TYPE == VTK_TETRA)
//      cell_size = 4;
//  else
//  if(VTK_CELL_TYPE == VTK_QUAD)
//      cell_size = 4;
//  else
//  if(VTK_CELL_TYPE == VTK_HEXAHEDRON)
//      cell_size = 8;

//  std::vector<vtkIdType>  temp_array(cell_size);

//  SegmentationType & segments = device.getSegmentation();

//  PointAccessorType pnt_acc = viennagrid::default_point_accessor(device.getCellComplex());

//  multiview->resetGrid();
//  MultiView::MultiGrid multigrid = multiview->getGrid();

//  vtkUnstructuredGrid*    usg;
//  vtkPoints*              points;

//  std::size_t si = 0;
//  for(SegmentationIteratorType sit = segments.begin();
//      sit != segments.end(); sit++)
//  {
//    // for each segment, we shall setup an unstructured vtk grid
//    // and add it as a new block to the general, central multi-block datastructure
//    //
//    usg = vtkUnstructuredGrid::New();

//    // transfer the segment's geometry information
//    //
//    points = vtkPoints::New();
//    VertexRange vertices = viennagrid::elements<VertexType>(*sit);
//    std::map<std::size_t,std::size_t>   indexMap;
//    int i = 0;
//    double array[3];
//    array[0] = 0.0;
//    array[1] = 0.0;
//    array[2] = 0.0; // important for 2d case: the z-component has to be 0.0, to be sure ..

//    for (VertexIterator vit = vertices.begin(); vit != vertices.end(); ++vit)
//    {
//      for(int dim = 0; dim < DIMG; dim++) // that should be automatically unrolled by the compiler as dimg is static ..
//          array[dim] = pnt_acc(*vit)[dim];
//      points->InsertNextPoint(array);
//      indexMap[vit->id().get()] = i++;
//      //indexMap[i++] = points->InsertNextPoint(array);
//    }
//    usg->SetPoints(points);
//    points->Delete();

//    // transfer the segment's topology information
//    //
//    CellRange cells = viennagrid::elements<CellType>(*sit);

//    for (CellIterator cit = cells.begin(); cit != cells.end(); ++cit)
//    {
//      vtkIdType cell_index = 0;

//      for (VertexOnCellIterator vocit = viennagrid::elements<VertexType>(*cit).begin(); // TODO make this 'unroll'able' ..
//           vocit != viennagrid::elements<VertexType>(*cit).end();
//           ++vocit)
//      {
//          temp_array[cell_index++] = indexMap.at(vocit->id().get());
//      }
//      usg->InsertNextCell (VTK_CELL_TYPE , cell_size, &temp_array[0] );
//    }

//    // now, as the grid representing the segment has been set up,
//    // add it as a new block to the central multi-block datastructure
//    //
//    multigrid->SetBlock(si++, usg);

//    // we don't need the usg anymore,
//    // delete everything, we'll start from scratch with the next segment ..
//    //
//    usg->Delete();
//  }

//  multiview->multigridModified();
//}

//inline void copy(Device2u& device, MultiView* multiview)
//{
//    copy(device, multiview, tag::viennagrid_domain(), VTK_TRIANGLE);
//}

//inline void copy(Device2s& device, MultiView* multiview)
//{
//    copy(device, multiview, tag::viennagrid_domain(), VTK_QUAD);
//}

//inline void copy(Device3u& device, MultiView* multiview)
//{
//    copy(device, multiview, tag::viennagrid_domain(), VTK_TETRA);
//}

//inline void copy(Device3s& device, MultiView* multiview)
//{
//    copy(device, multiview, tag::viennagrid_domain(), VTK_HEXAHEDRON);
//}

//template<typename DeviceT, typename SourceAccessorT, typename TargetAccessorT>
//inline void copy(DeviceT & device, SourceAccessorT source, TargetAccessorT target)
//{
//  typedef typename DeviceT::CellComplex                                                           CellComplexType;
//  typedef typename viennagrid::result_of::cell_tag<CellComplexType>::type                         CellTag;
//  typedef typename viennagrid::result_of::element<CellComplexType, CellTag>::type                 CellType;
//  typedef typename viennagrid::result_of::element_range<CellComplexType, CellTag>::type           CellRange;
//  typedef typename viennagrid::result_of::iterator<CellRange>::type                               CellIterator;

//  CellRange cells = viennagrid::elements<CellType>(device.getCellComplex());
//  for(CellIterator cit = cells.begin(); cit != cells.end(); cit++)
//  {
//    target(*cit) = source(*cit);
//  }

//}

//template<typename DomainT, typename AccessorT>
//inline void copy(DomainT     & domain,     AccessorT & acc,
//                 QuantitySet & quantities, Quantity  & quan)
//{
//    typedef typename DomainT::config_type                                                   DomainConfig;
//    typedef typename DomainConfig::cell_tag                                                 CellTag;
//    typedef typename viennagrid::result_of::const_ncell_range<DomainT, 0>::type             VertexContainer;
//    typedef typename viennagrid::result_of::iterator<VertexContainer>::type                 VertexIterator;
//    typedef typename viennagrid::result_of::const_ncell_range<DomainT, CellTag::dim>::type  CellContainer;
//    typedef typename viennagrid::result_of::iterator<CellContainer>::type                   CellIterator;


//    Data array = Data::New();

//    if(quan.tensor_lvl == SCALAR) array->SetNumberOfComponents(1);
//    else std::cerr << "Device quantity copy function is not implemented for this tensor level" << std::endl;

//    if(quan.cell_lvl == VERTEX)
//    {
//        VertexContainer const& vertices = viennagrid::ncells<0>(domain);
//        array->SetNumberOfValues(vertices.size());

//        quantities.modify(quantities.find(quan.acc_tuple()), Replace(array));

//        std::size_t i = 0;
//        for (VertexIterator vit = vertices.begin(); vit != vertices.end(); ++vit)
//        {
//            array->SetValue(i++, acc(*vit));
//        }
//    }
//    else if(quan.cell_lvl == CELL)
//    {
//        CellContainer const& cells = viennagrid::ncells<CellTag::dim>(domain);
//        array->SetNumberOfValues(cells.size());

//        quantities.modify(quantities.find(quan.acc_tuple()), Replace(array));

//        std::size_t i = 0;
//        for (CellIterator cit = cells.begin(); cit != cells.end(); ++cit)
//        {
//            array->SetValue(i++, acc(*cit));
//        }
//    }
//    else std::cerr << "Device quantity copy function is not implemented for this cell level" << std::endl;



//}



} // end viennamos





#endif // COPY_HPP

