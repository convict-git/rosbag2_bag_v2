// Copyright 2018, Bosch Software Innovations GmbH.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "rcpputils/filesystem_helper.hpp"
#include "rosbag2_storage/topic_metadata.hpp"
#include "rosbag_v2_storage_test_fixture.hpp"

using namespace ::testing;  // NOLINT
using namespace std::chrono_literals;  // NOLINT

namespace rosbag2_storage
{

bool operator!=(const TopicMetadata & lhs, const TopicMetadata & rhs)
{
  return !(lhs == rhs);
}

bool operator==(const TopicInformation & lhs, const TopicInformation & rhs)
{
  return lhs.topic_metadata == rhs.topic_metadata &&
         lhs.message_count == rhs.message_count;
}

bool operator!=(const TopicInformation & lhs, const TopicInformation & rhs)
{
  return !(lhs == rhs);
}

}  // namespace rosbag2_storage

TEST_F(RosbagV2StorageTestFixture, get_all_topics_and_types_returns_list_of_recorded_bag_file) {
  std::vector<rosbag2_storage::TopicMetadata> expected_topic_metadata = {
    {"/rosout", "rcl_interfaces/msg/Log", "rosbag_v2", ""},
    {"/test_topic", "std_msgs/msg/String", "rosbag_v2", ""},
    {"/test_topic2", "std_msgs/msg/String", "rosbag_v2", ""},
  };

  auto topic_metadata = storage_->get_all_topics_and_types();

  for (size_t i = 0; i < topic_metadata.size(); ++i) {
    EXPECT_STREQ(expected_topic_metadata[i].name.c_str(), topic_metadata[i].name.c_str());
    EXPECT_STREQ(expected_topic_metadata[i].type.c_str(), topic_metadata[i].type.c_str());
    EXPECT_STREQ(
      expected_topic_metadata[i].serialization_format.c_str(),
      topic_metadata[i].serialization_format.c_str());
  }
}

TEST_F(RosbagV2StorageTestFixture, get_metadata_returns_bagfile_description)
{
  std::vector<rosbag2_storage::TopicInformation> expected_topics_with_message_count = {
    {{"/rosout", "rcl_interfaces/msg/Log", "rosbag_v2", ""}, 3},
    {{"/test_topic", "std_msgs/msg/String", "rosbag_v2", ""}, 1},
    {{"/test_topic2", "std_msgs/msg/String", "rosbag_v2", ""}, 1}
  };

  auto bag_metadata = storage_->get_metadata();

  EXPECT_THAT(bag_metadata.version, Eq(2));
  EXPECT_THAT(bag_metadata.storage_identifier, StrEq("rosbag_v2"));
  EXPECT_THAT(bag_metadata.bag_size, Eq(9023u));
  EXPECT_THAT(bag_metadata.relative_file_paths, ElementsAre("test_bag.bag"));
  EXPECT_THAT(
    bag_metadata.starting_time,
    Eq(std::chrono::time_point<std::chrono::high_resolution_clock>(1543509813298505673ns)));
  EXPECT_THAT(bag_metadata.duration, Eq(268533408ns));
  EXPECT_THAT(bag_metadata.message_count, Eq(5u));
  EXPECT_THAT(
    bag_metadata.topics_with_message_count, SizeIs(expected_topics_with_message_count.size()));
  for (size_t i = 0; i < expected_topics_with_message_count.size(); ++i) {
    EXPECT_STREQ(
      expected_topics_with_message_count[i].topic_metadata.name.c_str(),
      bag_metadata.topics_with_message_count[i].topic_metadata.name.c_str());
    EXPECT_STREQ(
      expected_topics_with_message_count[i].topic_metadata.type.c_str(),
      bag_metadata.topics_with_message_count[i].topic_metadata.type.c_str());
    EXPECT_STREQ(
      expected_topics_with_message_count[i].topic_metadata.serialization_format.c_str(),
      bag_metadata.topics_with_message_count[i].topic_metadata.serialization_format.c_str());
  }
}

TEST_F(RosbagV2StorageTestFixture, has_next_only_counts_messages_with_ros2_counterpart)
{
  // There are only two messages that can be read
  auto n = storage_->get_metadata().message_count;
  for (size_t i = 0; i < n; ++i) {
    EXPECT_TRUE(storage_->has_next());
    storage_->read_next();
  }
  EXPECT_FALSE(storage_->has_next());
  EXPECT_FALSE(storage_->has_next());  // Once false, it stays false
}

TEST_F(RosbagV2StorageTestFixture, read_next_will_produce_messages_ordered_by_timestamp)
{
  EXPECT_TRUE(storage_->has_next());
  auto first_message = storage_->read_next();


  EXPECT_TRUE(storage_->has_next());
  auto second_message = storage_->read_next();

  EXPECT_THAT(second_message->time_stamp, Ge(first_message->time_stamp));
}

TEST_F(RosbagV2StorageTestFixture, get_topics_and_types_will_only_return_one_entry_per_topic)
{
  bag_path_ = (rcpputils::fs::path(database_path_) / "test_bag_multiple_connections.bag").string();
  storage_ = std::make_shared<rosbag2_bag_v2_plugins::RosbagV2Storage>();
  storage_->open(bag_path_, rosbag2_storage::storage_interfaces::IOFlag::READ_ONLY);

  std::vector<rosbag2_storage::TopicMetadata> expected_topic_metadata = {
    {"/rosout", "rcl_interfaces/msg/Log", "rosbag_v2", ""},
    {"/test_topic", "std_msgs/msg/String", "rosbag_v2", ""},
    {"/int_test_topic", "std_msgs/msg/Int32", "rosbag_v2", ""},
  };

  auto topic_metadata = storage_->get_all_topics_and_types();

  EXPECT_THAT(topic_metadata, SizeIs(expected_topic_metadata.size()));

  for (size_t i = 0; i < expected_topic_metadata.size(); ++i) {
    EXPECT_THAT(topic_metadata[i], expected_topic_metadata[i]);
  }
}
