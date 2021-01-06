import { logout, getInfo } from '@/api/user'
import { getToken, setToken, removeToken } from '@/utils/auth'
import { resetRouter } from '@/router'

const state = {
  token: getToken(),
  name: '',
  email: '',
  isAdmin: false,
  avatar: '',
  service: ''
}

const mutations = {
  SET_TOKEN: (state, token) => {
    state.token = token
  },
  SET_NAME: (state, name) => {
    state.name = name
  },
  SET_EMAIL: (state, email) => {
    state.email = email
  },
  SET_ISADMIN: (state, isAdmin) => {
    state.isAdmin = isAdmin
  },
  SET_AVATAR: (state, avatar) => {
    state.avatar = avatar
  },
  SET_SERVICE: (state, service) => {
    state.service = service
  }
}

const actions = {
  // user login
  login({ commit }, data) {
    commit('SET_TOKEN', data.token)
    commit('SET_SERVICE', data.service)
    setToken(data.token)
  },

  // get user info
  getInfo({ commit, state }) {
    return new Promise((resolve, reject) => {
      getInfo({ service: state.service }).then(response => {
        const data = response.Data

        if (!data) {
          reject('Verification failed, please Login again.')
        }
        const Name = data.Name
        const Email = data.Email
        const IsAdmin = data.IsAdmin
        commit('SET_NAME', Name)
        commit('SET_EMAIL', Email)
        commit('SET_ISADMIN', IsAdmin)
        commit('SET_AVATAR', 'https://wpimg.wallstcn.com/f778738c-e4f8-4870-b634-56703b4acafe.gif')
        resolve(data)
      }).catch(error => {
        reject(error)
      })
    })
  },

  // user logout
  logout({ commit, state }) {
    return new Promise((resolve, reject) => {
      logout(state.token).then(() => {
        commit('SET_TOKEN', '')
        removeToken()
        resetRouter()
        resolve()
      }).catch(error => {
        reject(error)
      })
    })
  },

  // remove token
  resetToken({ commit }) {
    return new Promise(resolve => {
      commit('SET_TOKEN', '')
      removeToken()
      resolve()
    })
  }
}

export default {
  namespaced: true,
  state,
  mutations,
  actions
}

