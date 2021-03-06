#include "optical_flow.h"

void MatchOpticalFlowFeatures(
    const cv::Mat& left_img, const std::pair<std::vector<cv::KeyPoint>, cv::Mat>& l_kps_descriptors,
    const cv::Mat& right_img,
    std::pair<std::vector<cv::KeyPoint>, cv::Mat>& r_kps_descriptors,
    std::vector<cv::DMatch>& matches) {

  const std::vector<cv::KeyPoint>& left_keypoints  = l_kps_descriptors.first;
  const std::vector<cv::KeyPoint>& right_keypoints = r_kps_descriptors.first;

  std::vector<cv::Point2f> l_pts;
  cv::KeyPoint::convert(left_keypoints, l_pts);
  cv::Mat left_img_k;
  cv::Mat right_img_k;
  cv::drawKeypoints(left_img, left_keypoints, left_img_k);
  cv::drawKeypoints(right_img, right_keypoints, right_img_k);
  // cv::imshow("left_key", left_img_k);
  // cv::waitKey(0);
  // cv::imshow("right_key", right_img_k);
  // cv::waitKey(0);
  std::vector<cv::Point2f> r_pts(l_pts.size());

  // making sure images are grayscale
  cv::Mat prevgray, gray;
  if (left_img.channels() == 3) {
    cvtColor(left_img, prevgray, CV_RGB2GRAY);
    cvtColor(right_img, gray, CV_RGB2GRAY);
  } else {
    prevgray = left_img;
    gray     = right_img;
  }

  std::vector<uchar> vstatus(l_pts.size());
  std::vector<float> verror(l_pts.size());

  // Run optical flow.
  calcOpticalFlowPyrLK(prevgray, gray, l_pts, r_pts, vstatus, verror);

  std::cout << "r_pts: " << r_pts.size() << " l_pts: " << l_pts.size() << " vs: " <<cv::countNonZero(vstatus) << std::endl;
  //!!  Create matches datastructure here.
  matches.clear();
  r_kps_descriptors.first.clear();
  for (size_t m = 0; m < l_pts.size(); ++m) {
    cv::DMatch mtch;
    mtch.queryIdx = m;
    mtch.trainIdx =m;
    mtch.imgIdx=m;
    mtch.distance=1.0f;
    matches.push_back(mtch);
    cv::KeyPoint kp;
    kp.pt = r_pts[m];
    r_kps_descriptors.first.push_back(kp);
  }
  return;

  std::vector<cv::Point2f> to_find;
  std::vector<int> to_find_back_idx;
  for (unsigned int i = 0; i < vstatus.size(); i++) {
    if (vstatus[i] && verror[i] < 12.0) {
      to_find_back_idx.push_back(i);
      to_find.push_back(r_pts[i]);
    } else {
      vstatus[i] = 0;
    }
  }

  std::vector<cv::KeyPoint> of_keypoints;

  // cv::KeyPoint::convert(to_find, of_keypoints);
  // cv::drawKeypoints(right_img, of_keypoints, right_img_k);
  // cv::imshow("right_key_of", right_img_k);
  // cv::waitKey(0);

  std::set<int> found_in_imgpts_r;
  cv::Mat to_find_flat = cv::Mat(to_find).reshape(1, to_find.size());

  std::vector<cv::Point2f> r_pts_to_find;
  cv::KeyPoint::convert(right_keypoints, r_pts_to_find);
  cv::Mat r_pts_flat = cv::Mat(r_pts_to_find).reshape(1, r_pts_to_find.size());

  std::vector<std::vector<cv::DMatch>> knn_matches;
  // FlannBasedMatcher matcher;
  cv::BFMatcher matcher(CV_L2);
  matcher.radiusMatch(to_find_flat, r_pts_flat, knn_matches, 2.0f);

  for (int i = 0; i < knn_matches.size(); i++) {
    cv::DMatch mtch;
    if (knn_matches[i].size() == 1) {
      mtch = knn_matches[i][0];
    } else if (knn_matches[i].size() > 1) {
      if (knn_matches[i][0].distance / knn_matches[i][1].distance < 0.7) {
        mtch = knn_matches[i][0];
      } else {
        continue;   // did not pass ratio test
      }
    } else {
      continue;   // no match
    }
    if (found_in_imgpts_r.find(mtch.trainIdx) == found_in_imgpts_r.end()) {   // prevent duplicates
      mtch.queryIdx =
          to_find_back_idx[mtch.queryIdx];   // back to original indexing of points for <i_idx>
      matches.push_back(mtch);
      found_in_imgpts_r.insert(mtch.trainIdx);
    }
  }
  std::cout << "OF matches: " << matches.size() << " l: " << left_keypoints.size()
            << " r: " << right_keypoints.size() << std::endl;
  /*
  // draw flow field
  cv::Mat img_matches;
  img_matches = left_img;
  // cv::cvtColor(left_img, img_matches, CV_GRAY2BGR);
  l_pts.clear();
  r_pts.clear();
  for (int i = 0; i < matches.size(); i++) {
    // if (i%2 != 0) {
    //				continue;
    //			}
    cv::Point l_pt = left_keypoints[(matches)[i].queryIdx].pt;
    cv::Point r_pt = right_keypoints[(matches)[i].trainIdx].pt;
    l_pts.push_back(l_pt);
    r_pts.push_back(r_pt);
    vstatus[i] = 1;
  }
  draw_arrows(img_matches, l_pts, r_pts, vstatus, verror);
  std::stringstream ss;
  ss << "flow_field_" <<  matches.size() << ".png";
  cv::imshow(ss.str(), img_matches);
  int c = cv::waitKey(0);
  if (c == 's') {
    cv::imwrite(ss.str(), img_matches);
  }
  cv::destroyWindow(ss.str());
  */
}
