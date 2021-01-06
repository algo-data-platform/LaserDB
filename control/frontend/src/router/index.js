import Vue from "vue";
import Router from "vue-router";

Vue.use(Router);

/* Layout */
import Layout from "@/layout";

/**
 * Note: sub-menu only appear when route children.length >= 1
 * Detail see: https://panjiachen.github.io/vue-element-admin-site/guide/essentials/router-and-nav.html
 *
 * hidden: true                   if set true, item will not show in the sidebar(default is false)
 * alwaysShow: true               if set true, will always show the root menu
 *                                if not set alwaysShow, when item has more than one children route,
 *                                it will becomes nested mode, otherwise not show the root menu
 * redirect: noRedirect           if set noRedirect will no redirect in the breadcrumb
 * name:'router-name'             the name is used by <keep-alive> (must set!!!)
 * meta : {
    roles: ['admin','editor']    control the page roles (you can set multiple roles)
    title: 'title'               the name show in sidebar and breadcrumb (recommend set)
    icon: 'svg-name'             the icon show in the sidebar
    breadcrumb: false            if set false, the item will hidden in breadcrumb(default is true)
    activeMenu: '/example/list'  if set path, the sidebar will highlight the path you set
  }
 */

/**
 * constantRoutes
 * a base page that does not have permission requirements
 * all roles can be accessed
 */
export const constantRoutes = [
  { path: "/404", component: () => import("@/views/404"), hidden: true },

  {
    path: "/",
    component: Layout,
    redirect: "/dashboard",
    children: [
      {
        path: "dashboard",
        name: "Dashboard",
        component: () => import("@/views/dashboard/index"),
        meta: { title: "Dashboard", icon: "dashboard" }
      }
    ]
  },
  {
    path: "/cluster",
    component: Layout,
    children: [
      {
        path: "cluster",
        name: "cluster",
        component: () => import("@/views/cluster/index"),
        meta: { title: "集群管理", icon: "form" }
      },
      {
        path: "dc",
        name: "dc",
        component: () => import("@/views/dc/index"),
        meta: { title: "数据中心", icon: "form" }
      },
      {
        path: "group",
        name: "group",
        component: () => import("@/views/group/index"),
        meta: { title: "组管理", icon: "form" }
      },
      // {
      //   path : 'shard',
      //   name : 'shard',
      //   component : () => import('@/views/service/index'),
      //   meta : {title : '分片管理', icon : 'form'}
      // },
      {
        path: "node",
        name: "node",
        component: () => import("@/views/node/index"),
        meta: { title: "节点管理", icon: "form" }
      },
      {
        path: "node_config",
        name: "node_config",
        component: () => import("@/views/node_config/index"),
        meta: { title: "节点配置", icon: "form" }
      },
      {
        path: "ansible_config",
        name: "ansible_config",
        component: () => import("@/views/ansible_config/index"),
        meta: { title: "Ansible 配置", icon: "form" }
      }
    ],
    meta: { title: "集群管理", icon: "tree" }
  },
  {
    path: "/database",
    component: Layout,
    children: [
      {
        path: "database",
        name: "database",
        component: () => import("@/views/database/index"),
        meta: { title: "数据库管理", icon: "form" }
      },
      {
        path: "table",
        name: "table",
        component: () => import("@/views/table/index"),
        meta: { title: "数据表管理", icon: "form" }
      },
      {
        path: "table_config",
        name: "table_config",
        component: () => import("@/views/table_config/index"),
        meta: { title: "表配置管理", icon: "form" }
      },
      {
        path: "table_config_template",
        name: "table_config_template",
        component: () => import("@/views/table_config_template/index"),
        meta: { title: "表配置模板", icon: "form" }
      }
    ],
    meta: { title: "数据管理", icon: "tree" }
  },
  {
    path: "/tools",
    component: Layout,
    children: [
      {
        path: "assign_table_shard",
        name: "assign_table_shard",
        component: () => import("@/views/assign_table_shard/index"),
        meta: { title: "分配表分片", icon: "form" }
      },
      {
        path: "keycheck",
        name: "keycheck",
        component: () => import("@/views/keycheck/index"),
        meta: { title: "Key 校验", icon: "form" }
      },
      {
        path: "version_change",
        name: "version_change",
        component: () => import("@/views/version_change/index"),
        meta: { title: "版本变更", icon: "form" }
      },
      {
        path: "proxy_config",
        name: "proxy_config",
        component: () => import("@/views/proxy_config/index"),
        meta: { title: "proxy配置", icon: "form" }
      }
    ],
    meta: { title: "便捷工具", icon: "tree" }
  },
  {
    path: "/machine_category",
    component: Layout,
    children: [
      {
        path: "machine_category",
        name: "machine_category",
        component: () => import("@/views/machine_category/index"),
        meta: { title: "机器资源分类", icon: "form" }
      },
      {
        path: "machine",
        name: "machine",
        component: () => import("@/views/machine/index"),
        meta: { title: "机器资源列表", icon: "form" }
      }
    ],
    meta: { title: "机器资源管理", icon: "tree" }
  },
  {
    path: "/ticket",
    component: Layout,
    redirect: "/ticket",
    children: [
      {
        path: "ticket",
        name: "ticket",
        component: () => import("@/views/ticket/index"),
        meta: { title: "工单管理", icon: "tree" }
      }
    ]
  },
  {
    path: "/report_form",
    component: Layout,
    children: [
      {
        path: "system_indices",
        name: "system_indices",
        component: () => import("@/views/system_indices/index"),
        meta: { title: "系统指数", icon: "form" }
      },
      {
        path: "table_metrics",
        name: "table_metrics",
        component: () => import("@/views/table_metrics/index"),
        meta: { title: "数据表信息", icon: "form" }
      },
      {
        path: "cluster_running_metrics",
        name: "cluster_running_metrics",
        component: () => import("@/views/cluster_running_metrics/index"),
        meta: { title: "集群运行时信息", icon: "form" }
      },
      {
        path: "resource_pool_running_metrics",
        name: "resource_pool_running_metrics",
        component: () => import("@/views/resource_pool_running_metrics/index"),
        meta: { title: "资源池运行时信息", icon: "form" }
      },
      {
        path: "group_running_metrics",
        name: "group_running_metrics",
        component: () => import("@/views/group_running_metrics/index"),
        meta: { title: "群组运行时信息", icon: "form" }
      },
      {
        path: "node_running_metrics",
        name: "node_running_metrics",
        component: () => import("@/views/node_running_metrics/index"),
        meta: { title: "节点运行时信息", icon: "form" }
      },
      {
        path: "node_physical_metrics",
        name: "node_physical_metrics",
        component: () => import("@/views/node_physical_metrics/index"),
        meta: { title: "节点物理资源使用情况", icon: "form" }
      }    
    ],
    meta: { title: "报表", icon: "tree" }
  },
  // 404 page must be placed at the end !!!
  { path: "*", redirect: "/404", hidden: true }
];

const createRouter = () =>
  new Router({
    // mode: 'history', // require service support
    scrollBehavior: () => ({ y: 0 }),
    routes: constantRoutes
  });

const router = createRouter();

// Detail see: https://github.com/vuejs/vue-router/issues/1234#issuecomment-357941465
export function resetRouter() {
  const newRouter = createRouter();
  router.matcher = newRouter.matcher; // reset router
}

export default router;
