import request from "@/utils/request";

export function ListDc(params) {
  return request({ url: "/dc/list", method: "get", params });
}

export function AddDc(dc) {
  return request({ url: "/dc/store", method: "post", data: dc });
}

export function UpdateDc(dc) {
  return request({ url: "/dc/update", method: "post", data: dc });
}
