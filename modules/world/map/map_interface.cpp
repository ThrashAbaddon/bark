// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.


#include <math.h>
#include <random>
#include <memory>
#include "modules/world/map/map_interface.hpp"

namespace modules {
namespace world {
namespace map {

using LineSegment = boost::geometry::model::segment<Point2d>;
bool MapInterface::interface_from_opendrive(
    const OpenDriveMapPtr &open_drive_map) {
  open_drive_map_ = open_drive_map;

  RoadgraphPtr roadgraph(new Roadgraph());
  roadgraph->Generate(open_drive_map);
  roadgraph_ = roadgraph;

  rtree_lane_.clear();
  for (auto &road : open_drive_map_->get_roads()) {
    for (auto &lane_section : road.second->get_lane_sections()) {
      for (auto &lane : lane_section->get_lanes()) {
        if (lane.second->get_lane_position() == 0)
          continue;
        LineSegment lane_segment(*lane.second->get_line().begin(),
                                 *(lane.second->get_line().end() - 1));
        rtree_lane_.insert(
          std::make_pair(lane_segment, lane.second));
      }
    }
  }

  bounding_box_ = open_drive_map_->bounding_box();
  return true;
}

bool MapInterface::FindNearestXodrLanes(const Point2d &point,
                                    const unsigned &num_lanes,
                                    std::vector<XodrLanePtr> &lanes,
                                    bool type_driving_only) const {
  std::vector<rtree_lane_value> results_n;
  if (type_driving_only) {
    rtree_lane_.query(
      boost::geometry::index::nearest(point, num_lanes) && boost::geometry::index::satisfies(is_lane_type),  // NOLINT
      std::back_inserter(results_n));
  } else {
    rtree_lane_.query(
      boost::geometry::index::nearest(point, num_lanes),
        std::back_inserter(results_n));
  }
  if (results_n.empty()) {
    return false;
  }
  lanes.clear();
  for (auto &result : results_n) {
    lanes.push_back(result.second);
  }
  return true;
}

XodrLanePtr MapInterface::FindXodrLane(const Point2d& point) const {
  XodrLanePtr lane;
  std::vector<XodrLanePtr> nearest_lanes;
  // TODO(@esterle): parameter (20) auslagern
  if (!FindNearestXodrLanes(point, 20, nearest_lanes, false)) {
    return nullptr;
  }
  for (auto &close_lane : nearest_lanes) {
    if (IsInXodrLane(point, close_lane->get_id())) {
      lane = close_lane;
      return lane;
    }
  }
  return nullptr;
}

bool MapInterface::IsInXodrLane(const Point2d &point, XodrLaneId id) const {
  std::pair<vertex_t, bool> v = roadgraph_->get_vertex_by_lane_id(id);
  if (v.second) {
    auto polygon = roadgraph_->get_lane_graph()[v.first].polygon;
    if (!polygon) {
      // found vertex has no polygon
      return false;
    } else {
      // found vertex has a polygon
      bool point_in_polygon = modules::geometry::Collide(*polygon, point);
      if (point_in_polygon) {
        return true;
      } else {
        return false;
      }
    }
  } else {
    // no vertex found
    return false;
  }
}


std::vector<PathBoundaries> MapInterface::ComputeAllPathBoundaries(
    const std::vector<XodrLaneId> &lane_ids) const
{
  std::vector<XodrLaneEdgeType> LANE_SUCCESSOR_EDGEs =
    {XodrLaneEdgeType::LANE_SUCCESSOR_EDGE};
  std::vector<std::vector<XodrLaneId>> all_paths =
    roadgraph_->find_all_paths_in_subgraph(LANE_SUCCESSOR_EDGEs, lane_ids);

  std::vector<PathBoundaries> all_path_boundaries;
  for (auto const &path : all_paths) {
    std::vector<std::pair<XodrLanePtr, XodrLanePtr>> path_boundaries;
    for (auto const &path_segment : path) {
      std::pair<XodrLanePtr, XodrLanePtr> lane_boundaries =
        roadgraph_->ComputeXodrLaneBoundaries(path_segment);
      path_boundaries.push_back(lane_boundaries);
    }
    all_path_boundaries.push_back(path_boundaries);
  }
  return all_path_boundaries;
}

std::pair<XodrLanePtr, bool> MapInterface::get_inner_neighbor(
  const XodrLaneId lane_id) const {
  std::pair<XodrLaneId, bool> inner_neighbor =
  roadgraph_->get_inner_neighbor(lane_id);
  if (inner_neighbor.second)
    return std::make_pair(roadgraph_->get_laneptr(inner_neighbor.first), true);
  return std::make_pair(nullptr, false);
}

std::pair<XodrLanePtr, bool> MapInterface::get_outer_neighbor(
  const XodrLaneId lane_id) const {
  std::pair<XodrLaneId, bool> outer_neighbor =
    roadgraph_->get_outer_neighbor(lane_id);
  if (outer_neighbor.second)
    return std::make_pair(roadgraph_->get_laneptr(outer_neighbor.first), true);
  return std::make_pair(nullptr, false);
}

std::vector<XodrLaneId> MapInterface::get_successor_lanes(
  const XodrLaneId lane_id) const {
  return roadgraph_->get_successor_lanes(lane_id);
}

void MapInterface::CalculateLaneCorridors(
  RoadCorridorPtr& road_corridor,
  const RoadPtr& road) {
  Lanes lanes = road->GetLanes();

  for (auto& lane : lanes) {
    // only add lane if it has not been added already
    if (road_corridor->GetLaneCorridor(lane.first) ||
        lane.second->get_lane_position() == 0)
      continue;

    LaneCorridorPtr lane_corridor = std::make_shared<LaneCorridor>();
    LanePtr current_lane = lane.second;
    float total_s = current_lane->GetCenterLine().length();
    lane_corridor->SetCenterLine(current_lane->GetCenterLine());
    lane_corridor->SetMergedPolygon(current_lane->GetPolygon());
    lane_corridor->SetLeftBoundary(
      current_lane->GetLeftBoundary().line_);
    lane_corridor->SetRightBoundary(
      current_lane->GetRightBoundary().line_);
    lane_corridor->SetLane(
      total_s,
      current_lane);
    // add initial lane
    road_corridor->SetLaneCorridor(current_lane->get_id(), lane_corridor);

    LanePtr next_lane = current_lane;
    for (;;) {
      next_lane = next_lane->GetNextLane();
      if (next_lane == nullptr)
        break;
      lane_corridor->GetCenterLine().ConcatenateLinestring(
        next_lane->GetCenterLine());
      lane_corridor->GetLeftBoundary().ConcatenateLinestring(
        next_lane->GetLeftBoundary().line_);
      lane_corridor->GetRightBoundary().ConcatenateLinestring(
        next_lane->GetRightBoundary().line_);
      lane_corridor->GetMergedPolygon().ConcatenatePolygons(
        next_lane->GetPolygon());

      total_s = lane_corridor->GetCenterLine().length();
      lane_corridor->SetLane(
        total_s,
        next_lane);
      // all following lanes should point to the same LaneCorridor
      road_corridor->SetLaneCorridor(next_lane->get_id(), lane_corridor);
    }
  }
}

void MapInterface::CalculateLaneCorridors(
  RoadCorridorPtr& road_corridor) {
  RoadPtr first_road = road_corridor->GetRoads()[0];
  for (auto& road : road_corridor->GetRoads()) {
    CalculateLaneCorridors(road_corridor, road.second);
  }
}

LanePtr MapInterface::GenerateRoadCorridorLane(const XodrLanePtr& xodr_lane) {
  LanePtr lane = std::make_shared<Lane>(xodr_lane);
  // polygons
  std::pair<PolygonPtr, bool> polygon_success =
    roadgraph_->ComputeXodrLanePolygon(xodr_lane->get_id());
  lane->SetPolygon(*polygon_success.first);
  return lane;
}

RoadPtr MapInterface::GenerateRoadCorridorRoad(const XodrRoadId& road_id) {
  XodrRoadPtr xodr_road = open_drive_map_->get_road(road_id);
  RoadPtr road = std::make_shared<Road>(xodr_road);
  Lanes lanes;
  for (auto& lane_section : xodr_road->get_lane_sections()) {
    for (auto& lane : lane_section->get_lanes()) {
      // TODO(@hart): only add driving lanes
      // if (lane.second->get_lane_type() == XodrLaneType::DRIVING)
      lanes[lane.first] = GenerateRoadCorridorLane(lane.second);
    }
  }
  road->SetLanes(lanes);
  return road;
}

void MapInterface::GenerateRoadCorridor(
  const std::vector<XodrRoadId>& road_ids,
  const XodrDrivingDirection& driving_direction) {
  std::size_t road_corridor_hash = RoadCorridor::GetHash(
    driving_direction, road_ids);

  // only compute if it has not been computed yet
  if (road_corridors_.count(road_corridor_hash) > 0)
    return;

  Roads roads;
  for (auto& road_id : road_ids)
    roads[road_id] = GenerateRoadCorridorRoad(road_id);

  // links can only be set once all roads have been calculated
  for (auto& road : roads) {
    // road successor
    RoadPtr next_road = GetNextRoad(road.first, roads, road_ids);
    road.second->SetNextRoad(next_road);
    for (auto& lane : road.second->GetLanes()) {
      // lane successor
      std::pair<XodrLaneId, bool> next_lane =
        roadgraph_->GetNextLane(road_ids, lane.first);
      if (next_lane.second && next_road)
        lane.second->SetNextLane(next_road->GetLane(next_lane.first));

      // left and right lanes
      LanePtr left_lane, right_lane;
      std::pair<XodrLaneId, bool> left_lane_id =
        roadgraph_->GetLeftLane(lane.first, driving_direction);
      if (left_lane_id.second) {
        left_lane = road.second->GetLane(left_lane_id.first);
        lane.second->SetLeftLane(left_lane);
      }

      std::pair<XodrLaneId, bool> right_lane_id =
        roadgraph_->GetRightLane(lane.first, driving_direction);
      if (right_lane_id.second) {
        right_lane = road.second->GetLane(right_lane_id.first);
        lane.second->SetRightLane(right_lane);
      }

      // set boundaries for lane
      std::pair<XodrLaneId, bool> left_boundary_lane_id =
        roadgraph_->GetLeftBoundary(lane.first, driving_direction);
      if (left_boundary_lane_id.second) {
        LanePtr left_lane_boundary = road.second->GetLane(
          left_boundary_lane_id.first);
        Boundary left_bound;
        left_bound.SetLine(left_lane_boundary->get_line());
        left_bound.SetType(left_lane_boundary->get_road_mark());
        lane.second->SetLeftBoundary(left_bound);
      }
      std::pair<XodrLaneId, bool> right_boundary_lane_id =
        roadgraph_->GetRightBoundary(lane.first, driving_direction);
      if (right_boundary_lane_id.second) {
        LanePtr right_lane_boundary = road.second->GetLane(
          right_boundary_lane_id.first);
        Boundary right_bound;
        right_bound.SetLine(right_lane_boundary->get_line());
        right_bound.SetType(right_lane_boundary->get_road_mark());
        lane.second->SetRightBoundary(right_bound);
      }

      // compute center line
      if (left_boundary_lane_id.second && right_boundary_lane_id.second)
        lane.second->SetCenterLine(
          ComputeCenterLine(lane.second->GetLeftBoundary().line_,
          lane.second->GetRightBoundary().line_));
    }
  }

  if (roads.size() == 0)
    return;
  RoadCorridorPtr road_corridor = std::make_shared<RoadCorridor>();
  road_corridor->SetRoads(roads);
  CalculateLaneCorridors(road_corridor);
  road_corridors_[road_corridor_hash] = road_corridor;
}

RoadCorridorPtr MapInterface::GenerateRoadCorridor(
  const modules::geometry::Point2d& start_point,
  const modules::geometry::Polygon& goal_region) {
  std::vector<XodrLanePtr> lanes;
  XodrLaneId goal_lane_id;
  bool nearest_start_lane_found = FindNearestXodrLanes(start_point, 1, lanes);
  bool nearest_goal_lane_found = XodrLaneIdAtPolygon(goal_region,
                                                     goal_lane_id);
  if (!nearest_start_lane_found || !nearest_goal_lane_found) {
    LOG(INFO) << "Could not generate road corridor based on geometric start and goal definitions.";  // NOLINT
    return nullptr;
  }
  const auto start_lane_id = lanes.at(0)->get_id();
  const XodrDrivingDirection driving_direction =
    lanes.at(0)->get_driving_direction();
  std::vector<XodrRoadId> road_ids = roadgraph_->FindRoadPath(start_lane_id,
                                                goal_lane_id);
  GenerateRoadCorridor(road_ids, driving_direction);
  return GetRoadCorridor(road_ids, driving_direction);
}

bool MapInterface::XodrLaneIdAtPolygon(
  const modules::geometry::Polygon&  polygon,
  XodrLaneId& found_lane_id) const {
  modules::geometry::Point2d goal_center(polygon.center_(0),
                                         polygon.center_(1));
  std::vector<opendrive::XodrLanePtr> nearest_lanes;
  if (FindNearestXodrLanes(goal_center, 1, nearest_lanes)) {
      found_lane_id = nearest_lanes[0]->get_id();
      return true;
  }
  LOG(INFO) << "No matching lane for goal definition found";
  return false;
}

RoadPtr MapInterface::GetNextRoad(
  const XodrRoadId& current_road_id,
  const Roads& roads,
  const std::vector<XodrRoadId>& road_ids) const {
  auto it = std::find(
    road_ids.begin(),
    road_ids.end(),
    current_road_id);
  if (road_ids.back() == current_road_id)
    return nullptr;
  return roads.at(*std::next(it, 1));
}

}  // namespace map
}  // namespace world
}  // namespace modules
