#include "training-data-manager.h"

#include <sstream>

const char kDefaultTrainingDataName[] = "Default";

// Useful macros that simplifies the coding.
// Always check label first!
#define CHECK_LABEL(label)                          \
    assert((label) > 0 && label <= num_classes_ &&  \
           "Label should be [1, num_classes_]");

#define CHECK_INDEX(label, index)                       \
    assert((index) < num_samples_per_label_[(label)] && \
           "Index exceeds the available samples");

TrainingDataManager::TrainingDataManager(uint32_t num_classes)
        : num_classes_(num_classes) {
    training_sample_names_.resize(num_classes + 1);
    num_samples_per_label_.resize(num_classes + 1, 0);

    for (uint32_t i = 0; i <= num_classes; i++) {
        default_label_names_.push_back(
            std::string("Label ") + std::to_string(i));
    }

    data_.setDatasetName(kDefaultTrainingDataName);
}

bool TrainingDataManager::setNumDimensions(uint32_t dim) {
    return data_.setNumDimensions(dim);
}

bool TrainingDataManager::setDatasetName(const std::string name) {
    return data_.setDatasetName(name);
}

bool TrainingDataManager::setDatasetName(const char* const name) {
    if ((name != NULL) && (name[0] == '\0')) {
        return data_.setDatasetName(std::string(name));
    }
    return false;
}

std::string TrainingDataManager::getSampleName(uint32_t label, uint32_t index) {
    CHECK_LABEL(label);
    CHECK_INDEX(label, index);

    const auto& name = training_sample_names_[label][index];
    if (name.first) {
        return name.second;
    } else {
        return default_label_names_[label] + " [" + std::to_string(index) + "]";
    }
}

bool TrainingDataManager::setSampleName(
    uint32_t label, uint32_t index, const std::string new_name) {
    CHECK_LABEL(label);

    auto& name = training_sample_names_[label][index];
    name.first = true;
    name.second = new_name;
    return true;
}

bool TrainingDataManager::addSample(
    uint32_t label, const GRT::MatrixDouble& sample) {
    CHECK_LABEL(label);

    // By default, set the name be <false, ""> so we will use the default name.
    training_sample_names_[label].push_back(
        std::make_pair(false, std::string()));
    data_.addSample(label, sample);
    num_samples_per_label_[label]++;

    return true;
}

std::string TrainingDataManager::getLabelName(uint32_t label) {
    CHECK_LABEL(label);
    return default_label_names_[label];
}

bool TrainingDataManager::setNameForLabel(const std::string name, uint32_t label) {
    CHECK_LABEL(label);
    default_label_names_[label] = name;
    return true;
}

GRT::MatrixDouble TrainingDataManager::getSample(uint32_t label, uint32_t index) {
    CHECK_LABEL(label);
    CHECK_INDEX(label, index);
    return data_.getClassData(label)[index].getData();
}

uint32_t TrainingDataManager::getNumSampleForLabel(uint32_t label) {
    CHECK_LABEL(label);
    assert(num_samples_per_label_[label] ==
           data_.getClassData(label).getNumSamples());
    return num_samples_per_label_[label];
}

bool TrainingDataManager::deleteSample(uint32_t label, uint32_t index) {
    CHECK_LABEL(label);
    CHECK_INDEX(label, index);

    // The implementation first remove all data and then add them back. This is
    // a temporary solution because GRT::TimeSeriesClassificationData doesn't
    // allow per-sample operation.
    GRT::TimeSeriesClassificationData data = data_.getClassData(label);
    data_.eraseAllSamplesWithClassLabel(label);

    for (uint32_t i = 0; i < data.getNumSamples(); i++) {
        if (i != index) {
            data_.addSample(label, data[i].getData());
        }
    }

    auto& names = training_sample_names_[label];
    names.erase(names.begin() + index);

    num_samples_per_label_[label]--;

    return true;
}

bool TrainingDataManager::deleteAllSamples() {
    for (uint32_t i = 0; i < num_classes_; i++) {
        data_.eraseAllSamplesWithClassLabel(i + 1);
        num_samples_per_label_[i + 1] = 0;
    }
    return true;
}

bool TrainingDataManager::deleteAllSamplesWithLabel(uint32_t label) {
    CHECK_LABEL(label);
    data_.eraseAllSamplesWithClassLabel(label);
    num_samples_per_label_[label] = 0;
    return true;
}

bool TrainingDataManager::relabelSample(
    uint32_t label, uint32_t index, uint32_t new_label) {
    CHECK_LABEL(label);
    CHECK_LABEL(new_label);
    CHECK_INDEX(label, index);

    GRT::MatrixDouble data = getSample(label, index);
    deleteSample(label, index);
    addSample(new_label, data);

    return true;
}

bool TrainingDataManager::trimSample(
    uint32_t label, uint32_t index, uint32_t start, uint32_t end) {
    CHECK_LABEL(label);
    CHECK_INDEX(label, index);

    GRT::TimeSeriesClassificationData data = data_.getClassData(label);
    data_.eraseAllSamplesWithClassLabel(label);

    for (uint32_t i = 0; i < data.getNumSamples(); i++) {
        if (i == index) {
            GRT::MatrixDouble sample = data[i].getData();
            GRT::MatrixDouble new_sample;

            for (uint32_t row = start; row <= end; row++) {
                new_sample.push_back(sample.getRowVector(row));
            }
            data_.addSample(label, new_sample);
        } else {
            data_.addSample(label, data[i].getData());
        }
    }
    return true;
}
