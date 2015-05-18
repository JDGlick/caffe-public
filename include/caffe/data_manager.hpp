//Copyright 2015 Zhicheng Yan

#ifndef CAFFE_DATA_MANAGER_HPP_
#define CAFFE_DATA_MANAGER_HPP_

#include <string>
#include <utility>
#include <vector>

#include "boost/scoped_ptr.hpp"
#include "hdf5.h"

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/data_transformer.hpp"
#include "caffe/data_variable_size_transformer.hpp"
#include "caffe/filler.hpp"
#include "caffe/internal_thread.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"
#include "caffe/util/db.hpp"

namespace caffe {

template<typename Dtype>
class Net;

template<typename Dtype>
class BaseDataManager: public InternalThread {
public:
	explicit BaseDataManager(const LayerParameter& data_layer_param,
			Net<Dtype> *net);
	~BaseDataManager();

	inline int GetDatumChannels() {
		return datum_channels_;
	}
	inline int GetDatumHeight() {
		return datum_height_;
	}
	inline int GetDatumWidth() {
		return datum_width_;
	}

	virtual void CreatePrefetchThread();
	virtual void JoinPrefetchThread();
	virtual void InternalThreadEntry() = 0;
	virtual void CopyFetchDataToConvThread(int replica_id,
			const vector<Blob<Dtype>*>& top) = 0;

protected:
	virtual void CreatePrefetchThread_() = 0;
	void SetBatchSize(int total_batch_size);

	LayerParameter layer_param_;
	Net<Dtype> *net_;

	shared_ptr<db::DB> db_;
	shared_ptr<db::Cursor> cursor_;
	shared_ptr<db::Transaction> transaction_;

	int datum_channels_, datum_height_, datum_width_;

	int forward_count_;
	boost::mutex forward_count_mutex_;
};

typedef struct {
	std::string img_name;
	int label;
} SelectiveItem;

//typedef std::pair<std::string, int> ItemNameLabel;

template<typename Dtype>
class DataManager: public BaseDataManager<Dtype> {
public:
	explicit DataManager(const LayerParameter& data_layer_param, Net<Dtype> *net);
	~DataManager();

	virtual void InternalThreadEntry();
	virtual void CopyFetchDataToConvThread(int replica_id,
			const vector<Blob<Dtype>*>& top);

protected:
	virtual void CreatePrefetchThread_();

	Blob<Dtype> prefetch_data_;
	Blob<Dtype> prefetch_label_;
	Blob<Dtype> transformed_data_;


	bool output_labels_;
	TransformationParameter transform_param_;
	DataTransformer<Dtype> data_transformer_;
	std::string selective_list_fn_;
	std::vector<SelectiveItem> selective_list_;
	int selective_list_cursor_;
};

template<typename Dtype>
class DataVariableSizeManager: public BaseDataManager<Dtype> {
public:
	explicit DataVariableSizeManager(const LayerParameter& data_layer_param, Net<Dtype> *net);
	~DataVariableSizeManager();

	virtual void InternalThreadEntry();
	virtual void CopyFetchDataToConvThread(int replica_id,
			const vector<Blob<Dtype>*>& top);
protected:
	virtual void CreatePrefetchThread_();

	/*
	 * prefetch_data height and width are set to the maximum height/width across all batches
	 * */
	Blob<Dtype> prefetch_data_;
	std::vector<Blob<Dtype>* > prefetch_data_reorganized_;
	Blob<Dtype> prefetch_data_size_;
	Blob<Dtype> replicas_batch_data_max_size_;

	Blob<Dtype> prefetch_label_;
	Blob<Dtype> transformed_data_;



	int datum_max_pixel_num_;

	bool output_labels_;
	TransformationParameter transform_param_;
	DataVariableSizeTransformer<Dtype> data_transformer_;
	std::string selective_list_fn_;
	std::vector<SelectiveItem> selective_list_;
	int selective_list_cursor_;
};


}  // namespace caffe

#endif  // CAFFE_DATA_MANAGER_HPP_
