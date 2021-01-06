import Mock from 'mockjs'

const data = Mock.mock({
  'items|30': [{
    'id|+1': 1000,
    name: '@sentence(1, 2)',
    dimName: '@sentence(2, 3)',
    desc: '@sentence(10, 20)'
  }]
})

export default [
  {
    url: '/feature/list',
    type: 'get',
    response: config => {
      const items = data.items
      return {
        code: 20000,
        data: {
          total: items.length,
          items: items
        }
      }
    }
  }
]
