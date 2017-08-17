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
static const ChargingAddresses NO_CHARGING_ADDRESSES = ChargingAddresses({}, {});
static const std::vector<std::string> IMPI = { "impi@example.com" };

// Not valid - just dummy data for testing.
static const std::string SERVICE_PROFILE = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><ServiceProfile></ServiceProfile>";

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

  int now = time(0);

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPI,
                               RegistrationState::REGISTERED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               now);

  EXPECT_EQ(Store::Status::OK,
            impu_store->set_impu(default_impu, 0));

  delete default_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetDefaultImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int now = time(0);

  ImpuStore::DefaultImpu* default_impu =
    new ImpuStore::DefaultImpu(IMPU,
                               NO_ASSOCIATED_IMPUS,
                               IMPI,
                               RegistrationState::REGISTERED,
                               NO_CHARGING_ADDRESSES,
                               SERVICE_PROFILE,
                               0L,
                               now);
  impu_store->set_impu(default_impu, 0);

  delete default_impu;

  ImpuStore::Impu* got_impu = impu_store->get_impu(IMPU.c_str(), 0L);

  ASSERT_NE(nullptr, got_impu);
  ASSERT_EQ(IMPU, got_impu->impu);
  ASSERT_TRUE(got_impu->is_default_impu());
  ASSERT_EQ(now, got_impu->expiry);

  ImpuStore::DefaultImpu* got_default_impu =
    dynamic_cast<ImpuStore::DefaultImpu*>(got_impu);

  ASSERT_NE(nullptr, got_default_impu);

  ASSERT_EQ(IMPI, got_default_impu->impis);
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

  int now = time(0);

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  now);

  impu_store->set_impu(assoc_impu, 0);

  delete assoc_impu;
  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, GetAssociatedImpu)
{
  LocalStore* local_store = new LocalStore();
  ImpuStore* impu_store = new ImpuStore(local_store);

  int now = time(0);

  ImpuStore::AssociatedImpu* assoc_impu =
    new ImpuStore::AssociatedImpu(ASSOC_IMPU,
                                  IMPU,
                                  0L,
                                  now);

  impu_store->set_impu(assoc_impu, 0);

  delete assoc_impu;

  ImpuStore::Impu* got_impu = impu_store->get_impu(ASSOC_IMPU, 0);

  ASSERT_NE(nullptr, got_impu);
  ASSERT_EQ(ASSOC_IMPU, got_impu->impu);
  ASSERT_FALSE(got_impu->is_default_impu());
  ASSERT_EQ(now, got_impu->expiry);

  ImpuStore::AssociatedImpu* got_associated_impu =
    dynamic_cast<ImpuStore::AssociatedImpu*>(got_impu);

  ASSERT_NE(nullptr, got_associated_impu);
  ASSERT_EQ(IMPU, got_associated_impu->default_impu);

  delete got_impu;

  delete impu_store;
  delete local_store;
}

TEST_F(ImpuStoreTest, ImpuFromDataEmpty)
{
  std::string data;
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}

TEST_F(ImpuStoreTest, ImpuFromDataIncorrectVersion)
{
  std::string data;
  data.push_back((char) -1);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}

TEST_F(ImpuStoreTest, ImpuFromDataVersion0NoLengthOrData)
{
  std::string data;
  data.push_back((char) 0);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}

TEST_F(ImpuStoreTest, ImpuFromDataVersion0TooLong)
{
  std::string data;
  data.push_back((char) 0);
  encode_varbyte(INT_MAX + 1L, data);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}

TEST_F(ImpuStoreTest, ImpuFromDataVersion0RunOffEnd)
{
  std::string data;
  data.push_back((char) 0);
  data.push_back((char) 0x80);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}

TEST_F(ImpuStoreTest, ImpuFromDataVersion0InvalidCompressData)
{
  std::string data;
  data.push_back((char) 0);
  data.push_back((char) 0x1);
  data.push_back((char) 0xFF);
  ASSERT_EQ(nullptr, ImpuStore::Impu::from_data(IMPU, data, 0));
}
