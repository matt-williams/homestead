/**
 * @file impu_store_test.cpp UT for IMPU Store
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
 */

#include "impu_store.h"
#include "localstore.h"
#include "test_interposer.hpp"
#include "test_utils.hpp"

static const std::string IMPU = "sip:impu@example.com";
static const std::string ASSOC_IMPU = "sip:assoc_impu@example.com";
static const std::vector<std::string> NO_ASSOCIATED_IMPUS;
static const std::vector<std::string> IMPUS = { IMPU };
static const ChargingAddresses NO_CHARGING_ADDRESSES = ChargingAddresses({}, {});
static const std::string IMPI = "impi@example.com";
static const std::vector<std::string> IMPIS = { IMPI };

// Not valid - just dummy data for testing.
static const std::string SERVICE_PROFILE = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><ServiceProfile></ServiceProfile>";

static const std::string INVALID_JSON = "{";
static const std::string JSON_ARRAY = "[]";

class ImpuStoreTest : public testing::Test
{
  static void SetUpTestCase()
  {
    cwtest_completely_control_time();
  }

  static void TearDownTestCase()
  {
    cwtest_reset_time();
  }
};

TEST_F(ImpuStoreTest, Constructor)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, EncodeDecodeSmallVarByte)
{
  std::string data;
  encode_varbyte(2, data);

  // 1 byte should take up 1 byte
  EXPECT_EQ(1, data.size());

  size_t offset = 0;

  EXPECT_EQ(2, decode_varbyte(data, offset));
}

TEST_F(ImpuStoreTest, SetDefaultImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPIS,
                               RegistrationState::REGISTERED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               expiry,
                               impu_store);

  EXPECT_EQ(Store::Status::OK,
            impu_store->set_impu(default_impu, 0));

  delete default_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, SetUnregisteredDefaultImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPIS,
                               RegistrationState::UNREGISTERED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               expiry,
                               impu_store);

  EXPECT_EQ(Store::Status::OK,
            impu_store->set_impu(default_impu, 0));

  delete default_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, SetInvalidRegistrationStateDefaultImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPIS,
                               RegistrationState::UNCHANGED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               expiry,
                               impu_store);

  EXPECT_EQ(Store::Status::OK,
            impu_store->set_impu(default_impu, 0));

  delete default_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetImpuInvalidData)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);
  local_store->set_data("impu",
                        IMPU,
                        INVALID_JSON,
                        0,
                        1,
                        0L);

  ImpuStore::Impu* impu = nullptr;
  Store::Status status = impu_store->get_impu(IMPU, impu, 0L);
  ASSERT_EQ(nullptr, impu);
  ASSERT_EQ(status, Store::Status::ERROR);

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetDefaultImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPIS,
                               RegistrationState::REGISTERED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               expiry,
                               impu_store);
  impu_store->set_impu(default_impu, 0);

  delete default_impu;

  ImpuStore::Impu* got_impu = nullptr;
  Store::Status status = impu_store->get_impu(IMPU.c_str(), got_impu, 0L);

  ASSERT_EQ(status, Store::Status::OK);
  ASSERT_NE(nullptr, got_impu);
  ASSERT_EQ(IMPU, got_impu->impu);
  ASSERT_TRUE(got_impu->is_default_impu());
  ASSERT_EQ(expiry, got_impu->expiry);

  ImpuStore::DefaultImpu* got_default_impu =
    dynamic_cast<ImpuStore::DefaultImpu*>(got_impu);

  ASSERT_NE(nullptr, got_default_impu);

  ASSERT_EQ(IMPIS, got_default_impu->impis);
  ASSERT_EQ(NO_ASSOCIATED_IMPUS, got_default_impu->associated_impus);
  ASSERT_EQ(NO_CHARGING_ADDRESSES.ccfs, got_default_impu->charging_addresses.ccfs);
  ASSERT_EQ(NO_CHARGING_ADDRESSES.ecfs, got_default_impu->charging_addresses.ecfs);
  ASSERT_EQ(SERVICE_PROFILE, got_default_impu->service_profile);

  delete got_default_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, SetAssociatedImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  expiry,
                                  impu_store);

  ASSERT_EQ(Store::Status::OK,
            impu_store->set_impu(assoc_impu, 0));

  delete assoc_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetAssociatedImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  expiry,
                                  impu_store);

  impu_store->set_impu(assoc_impu, 0);

  delete assoc_impu;

  ImpuStore::Impu* got_impu = nullptr;
  Store::Status status = impu_store->get_impu(ASSOC_IMPU, got_impu, 0);

  ASSERT_EQ(status, Store::Status::OK);
  ASSERT_NE(nullptr, got_impu);
  ASSERT_EQ(ASSOC_IMPU, got_impu->impu);
  ASSERT_FALSE(got_impu->is_default_impu());
  ASSERT_EQ(expiry, got_impu->expiry);

  ImpuStore::AssociatedImpu* got_associated_impu =
    dynamic_cast<ImpuStore::AssociatedImpu*>(got_impu);

  ASSERT_NE(nullptr, got_associated_impu);
  ASSERT_EQ(IMPU, got_associated_impu->default_impu);

  delete got_impu;

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, SetAssociatedImpiMapping)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::ImpiMapping* mapping =
    new ImpuStore::ImpiMapping(IMPI,
                               IMPUS,
                               0L,
                               expiry);

  ASSERT_EQ(Store::Status::OK,
            impu_store->set_impi_mapping(mapping, 0));

  delete mapping;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, DeleteAssociatedImpiMapping)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::ImpiMapping* mapping =
    new ImpuStore::ImpiMapping(IMPI,
                               IMPUS,
                               0L,
                               expiry);

  impu_store->set_impi_mapping(mapping, 0);
  ASSERT_EQ(Store::Status::OK,
            impu_store->delete_impi_mapping(mapping, 0));

  delete mapping;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetAssociatedImpiMapping)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::ImpiMapping* mapping =
    new ImpuStore::ImpiMapping(IMPI,
                               IMPUS,
                               0L,
                               expiry);

  impu_store->set_impi_mapping(mapping, 0);

  delete mapping;

  ImpuStore::ImpiMapping* got_mapping = nullptr;
  Store::Status status = impu_store->get_impi_mapping(IMPI, got_mapping, 0);

  ASSERT_EQ(status, Store::Status::OK);
  ASSERT_NE(nullptr, got_mapping);
  ASSERT_EQ(IMPI, got_mapping->impi);
  ASSERT_TRUE(got_mapping->has_default_impu(IMPU));
  ASSERT_EQ(expiry, got_mapping->get_expiry());

  delete got_mapping;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, ImpuFromDataEmpty)
{
  std::string data;
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreTest, ImpuFromDataIncorrectVersion)
{
  std::string data;
  data.push_back((char) -1);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

class ImpuStoreVersion0Test : public ImpuStoreTest
{
  void SetUp()
  {
    data.push_back((char) 0);
  }

  void TearDown()
  {
    data.clear();
  }

  std::string data;
};

TEST_F(ImpuStoreVersion0Test, NoLengthOrData)
{
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreVersion0Test, TooLong)
{
  encode_varbyte(INT_MAX + 1L, data);

  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreVersion0Test, RunOffEnd)
{
  data.push_back((char) 0x80);

  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreVersion0Test, InvalidCompressData)
{
  data.push_back((char) 0x1);
  data.push_back((char) 0xFF);

  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreVersion0Test, InvalidJson)
{
  encode_varbyte(INVALID_JSON.size(), data);
  char* buffer = nullptr;
  int comp_size;
  ImpuStore::Impu::compress_data_v0(INVALID_JSON, buffer, comp_size);
  data.append(buffer, comp_size);
  free(buffer);

  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
}

TEST_F(ImpuStoreVersion0Test, NotJsonObject)
{
  encode_varbyte(JSON_ARRAY.size(), data);
  char* buffer;
  int comp_size;
  ImpuStore::Impu::compress_data_v0(JSON_ARRAY, buffer, comp_size);
  data.append(buffer, comp_size);

  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0, nullptr));
  free(buffer);
}

TEST_F(ImpuStoreVersion0Test, BufferResize)
{
  std::string data;
  int length = 126 /* ~ */ - 34 /* # */;

  for (int i = 0; i < 1000000; ++i)
  {
    data.push_back((i % length) + 34);
  }

  char* buffer = nullptr;
  int comp_size;
  ImpuStore::Impu::compress_data_v0(data, buffer, comp_size);
  free(buffer);
}

TEST_F(ImpuStoreTest, ImpiMappingInvalidJson)
{
  ASSERT_EQ(nullptr,
            ImpuStore::ImpiMapping::from_data(IMPI,
                                              INVALID_JSON,
                                              0L));
}

TEST_F(ImpuStoreTest, ImpiMappingNotJsonObject)
{
  ASSERT_EQ(nullptr,
            ImpuStore::ImpiMapping::from_data(IMPI,
                                              JSON_ARRAY,
                                              0L));
}

TEST_F(ImpuStoreTest, ImpuNotFound)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  ImpuStore::Impu* got_impu = nullptr;
  Store::Status status = impu_store->get_impu(IMPU, got_impu, 0L);
  ASSERT_EQ(status, Store::Status::NOT_FOUND);
  ASSERT_EQ(nullptr, got_impu);

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, ImpiMappingNotFound)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  ImpuStore::ImpiMapping* mapping = nullptr;
  Store::Status status = impu_store->get_impi_mapping(IMPI, mapping, 0L);
  ASSERT_EQ(nullptr, mapping);
  ASSERT_EQ(status, Store::Status::NOT_FOUND);

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, ImpiMappingBadJson)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);
  local_store->set_data("impi_mapping",
                        IMPI,
                        INVALID_JSON,
                        0,
                        1,
                        0L);

  ImpuStore::ImpiMapping* mapping = nullptr;
  Store::Status status = impu_store->get_impi_mapping(IMPI, mapping, 0L);
  ASSERT_EQ(nullptr, mapping);
  ASSERT_EQ(status, Store::Status::ERROR);

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, ImpuSetWithoutCas)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  expiry,
                                  impu_store);

  ASSERT_EQ(Store::Status::OK,
            impu_store->set_impu_without_cas(assoc_impu, 0));

  delete assoc_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, DeleteImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int expiry = time(0) + 1;

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  expiry,
                                  impu_store);

  ASSERT_EQ(Store::Status::OK,
            impu_store->delete_impu(assoc_impu, 0));

  delete assoc_impu;
  delete impu_store;
  delete local_store;
}
