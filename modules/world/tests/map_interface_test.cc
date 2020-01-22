// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "gtest/gtest.h"
#include "modules/world/map/map_interface.hpp"
#include "modules/models/tests/make_test_world.hpp"

TEST(query_lanes, map_interface) {
  using namespace modules::world::opendrive;
  using namespace modules::world::map;
  using namespace modules::geometry;

  MapInterface map_interface =
    modules::models::tests::make_two_lane_map_interface();

  std::vector<XodrLanePtr> nearest_lanes;
  bool success = map_interface.FindNearestXodrLanes(
    Point2d(0, 0), 2, nearest_lanes);
  EXPECT_TRUE(success);
  EXPECT_EQ(nearest_lanes.size(), 2);

  success = map_interface.FindNearestXodrLanes(Point2d(0, 0), 3, nearest_lanes);
  EXPECT_TRUE(success);
  EXPECT_EQ(nearest_lanes.size(), 2);  // there exist only two lanes
}


TEST(point_in_lane, map_interface) {
  using namespace modules::world::opendrive;
  using namespace modules::world::map;
  using namespace modules::geometry;

  OpenDriveMapPtr open_drive_map(new OpenDriveMap());
  //! ROAD 1
  PlanViewPtr p(new PlanView());
  p->add_line(Point2d(0.0f, 0.0f), 0.0f, 10.0f);

  //! XodrLane-Section 1
  XodrLaneSectionPtr ls(new XodrLaneSection(0.0));

  //! PlanView
  XodrLaneOffset off0 = {0.0f, 0.0f, 0.0f, 0.0f};
  XodrLaneWidth lane_width_0 = {0, 10, off0};
  XodrLanePtr lane0 =
    create_lane_from_lane_width(0, p->get_reference_line(), lane_width_0, 0.05);
  lane0->set_lane_type(XodrLaneType::DRIVING);

  XodrLaneOffset off = {1.0f, 0.0f, 0.0f, 0.0f};
  XodrLaneWidth lane_width_1 = {0, 10, off};

  //! XodrLanes
  XodrLanePtr lane1 = create_lane_from_lane_width(
    -1, p->get_reference_line(), lane_width_1, 0.05);
  lane1->set_lane_type(XodrLaneType::DRIVING);
  XodrLanePtr lane2 = create_lane_from_lane_width(
    1, p->get_reference_line(), lane_width_1, 0.05);
  lane2->set_lane_type(XodrLaneType::DRIVING);
  XodrLanePtr lane3 = create_lane_from_lane_width(
    2, lane2->get_line(), lane_width_1, 0.05);
  lane3->set_lane_type(XodrLaneType::DRIVING);

  ls->add_lane(lane0);
  ls->add_lane(lane1);
  ls->add_lane(lane2);
  ls->add_lane(lane3);

  XodrRoadPtr r(new XodrRoad("highway", 100));
  r->set_plan_view(p);
  r->add_lane_section(ls);

  open_drive_map->add_road(r);

  MapInterface map_interface;
  map_interface.set_open_drive_map(open_drive_map);

  std::vector<XodrLanePtr> nearest_lanes;
  Point2d point = Point2d(0.5, 0.5);
  bool success = map_interface.FindNearestXodrLanes(point, 3, nearest_lanes);

  success = map_interface.IsInXodrLane(point, (nearest_lanes.at(0))->get_id());
  EXPECT_FALSE(success);

  success = map_interface.IsInXodrLane(point, (nearest_lanes.at(1))->get_id());
  EXPECT_TRUE(success);

  success = map_interface.IsInXodrLane(point, (nearest_lanes.at(2))->get_id());
  EXPECT_FALSE(success);
}